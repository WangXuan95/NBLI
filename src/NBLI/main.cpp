#include <ctime>
#include <cstdlib>

#include "../FileIO.h"
#include "../imageio/imageio.h"
#include "NBLI.h"


const char *USAGE = 
  "|----------------------------------------------------------------------------------|\n"
  "| NBLI : new-bee lossless image codec (single-threaded, v0.4, 202409)              |\n"
  "|   by https://github.com/WangXuan95/                                              |\n"
  "|----------------------------------------------------------------------------------|\n"
  "| Usage:                                                                           |\n"
  "|   NBLI [-switches]  <in1> [-o <out1>]  [<in2> [-o <out2]]  ...                   |\n"
  "|                                                                                  |\n"
  "| To compress:                                                                     |\n"
  "|   <in>  can be .pgm, .ppm, .pnm, or .png                                         |\n"
  "|   <out> can be .nbli. It will be generated if not specified.                     |\n"
  "|                                                                                  |\n"
  "| To decompress:                                                                   |\n"
  "|   <in>  can be .nbli                                                             |\n"
  "|   <out> can be .pgm, .ppm, .pnm, or .png. It will be generated if not specified. |\n"
  "|                                                                                  |\n"
  "| switches: -v   : verbose                                                         |\n"
  "|           -f   : force overwrite of output file                                  |\n"
  "|           -x   : putting CRC32 when compressing                                  |\n"
  "|           -0~7 : distortion level. 0=lossless (default), 1~7=lossy               |\n"
  "|           -g   : use golomb arithmetic coding tree instead of ANS coding (slower)|\n"
  "|           -a   : use advanced predictor (extremely slow)                         |\n"
  "|----------------------------------------------------------------------------------|\n"
  "\n";



// return:   true  : match
//           false : mismatch
inline static bool matchSuffixIgnoringCase (const char *string, const char *suffix) {
    #define  TO_LOWER(c)   ((((c) >= 'A') && ((c) <= 'Z')) ? ((c)+32) : (c))
    const char *p1, *p2;
    for (p1=string; *p1; p1++);
    for (p2=suffix; *p2; p2++);
    while (TO_LOWER(*p1) == TO_LOWER(*p2)) {
        if (p2 <= suffix)
            return true;
        if (p1 <= string)
            return false;
        p1 --;
        p2 --;
    }
    return false;
}


inline static void replaceFileSuffix (char *p_dst, const char *p_src, const char *p_suffix) {
    char *p_base = p_dst;
    
    for (; *p_src; p_src++)         // traversal p_src
        *(p_dst++) = *p_src;        // copy p_src to p_dst
    *p_dst = '\0';
    
    for (char *p=p_dst; ; p--) {    // reverse traversal p_dst
        if (p < p_base || *p=='/' || *p=='\\') {
            break;
        }
        if (*p == '.') {
            p_dst = p;
            break;
        }
    }
    
    *(p_dst++) = '.';
    
    for (; *p_suffix; p_suffix++) { // traversal p_suffix
        *(p_dst++) = *p_suffix;     // copy p_suffix to p_dst
    }
    *p_dst = '\0';
}


#define MAX_N_FILE 999


inline static void parseCommand (
    int  argc, char **argv,
    bool switches[128],
    int  &n_file,
    char *src_fnames[MAX_N_FILE],
    char *dst_fnames[MAX_N_FILE]
) {
    for (int i=0; i<128; i++)
        switches[i] = false;
    
    for (int i=0; i<MAX_N_FILE; i++)
        src_fnames[i] = dst_fnames[i] = NULL;
    
    bool next_is_dst = false;
    
    n_file = -1;
    
    for (int i=1; i<argc; i++) {
        char *arg = argv[i];
        
        if      (arg[0] == '-') {                   // parse switches
            
            for (arg++ ; *arg ; arg++) {
                if (0<= (int)(*arg) && (int)(*arg) < 128)
                    switches[(int)(*arg)] = true;
                
                if (*arg == 'o')
                    next_is_dst = true;
            }
            
        } else if (n_file+1 < MAX_N_FILE) {   // parse file names
            
            if (next_is_dst) {
                next_is_dst = false;
                
                dst_fnames[n_file] = arg;
            } else {
                n_file++;
                
                src_fnames[n_file] = arg;
            }
        }
    }
    
    n_file++;
}


#define  ERROR(error_message,fname) {   \
    printf("***ERROR: ");               \
    printf((error_message), (fname));   \
    printf("\n");                       \
    continue;                           \
}


int main (int argc, char **argv) {
    int n_file, n_compressed=0, n_decompressed=0;
    
    bool switches[128];
    char *src_fnames[MAX_N_FILE], *dst_fnames[MAX_N_FILE];
    
    parseCommand(argc, argv, switches, n_file, src_fnames, dst_fnames);
    
    const bool verbose       = switches['V'] || switches['v'];
    const bool force_write   = switches['F'] || switches['f'];
    const bool enable_crc    = switches['X'] || switches['x'];
    const bool enable_golomb = switches['G'] || switches['g'];
    const bool enable_avp    = switches['A'] || switches['a'];
    const int16_t enc_near   = switches['7']?7:switches['6']?6:switches['5']?5:switches['4']?4:switches['3']?3:switches['2']?2:switches['1']?1:0;
    
    if (n_file <= 0) {
        printf(USAGE);
        return 1;
    }
    
    
    for (int i_file=0; i_file<n_file; i_file++) {
        if (verbose && n_file > 1) {
            printf("(%d/%d) ", i_file+1, n_file);
            fflush(stdout);
        }
        
        char *p_dst_fname = dst_fnames[i_file];
        char *p_src_fname = src_fnames[i_file];
        
        uint32_t height, width, crc32=(enable_crc?1:0);
        size_t   comp_size;
        int16_t  near=enc_near;
        int      i_is_rgb;
        bool     is_rgb, use_golomb=enable_golomb, use_avp=enable_avp;
        uint8_t *in_buf = NULL;
        
        double start_time = (double)clock();
        
        if (in_buf == NULL) in_buf = loadPNMImageFile(p_src_fname, &i_is_rgb, &height, &width);
        if (in_buf == NULL) in_buf = loadPNGImageFile(p_src_fname, &i_is_rgb, &height, &width);
        is_rgb = (bool)i_is_rgb;
        
        if (in_buf != NULL) {
            if (verbose) {
                printf("%s (%dx%d, %s) ", p_src_fname, width, height, (is_rgb?"RGB":"Gray"));
                fflush(stdout);
            }
            
            uint8_t *out_buf = NBLIcompress(comp_size, in_buf, is_rgb, height, width, use_golomb, use_avp, near, crc32);
            
            free(in_buf);
            
            if (out_buf == NULL)
                ERROR("compress %s failed", p_src_fname);
            
            if (p_dst_fname == NULL) {
                static char dst_fname_buffer [8192];
                p_dst_fname = dst_fname_buffer;
                replaceFileSuffix(p_dst_fname, p_src_fname, "nbli");
            }
            
            if (verbose) {
                printf("-> %s (%lldB) ", p_dst_fname, (long long int)comp_size);
                fflush(stdout);
            }
            
            if (!force_write && fileExist(p_dst_fname))
                ERROR("%s already exist", p_dst_fname);
            
            if (writeBytesToFile(p_dst_fname, out_buf, comp_size)) {
                delete out_buf;
                ERROR("write %s failed", p_dst_fname);
            }
            
            delete[] out_buf;
            
            n_compressed ++;
        
        } else {
            in_buf = loadBytesFromFile(p_src_fname, comp_size);
            
            if (in_buf == NULL)
                ERROR("open %s failed", p_src_fname);
            
            if (verbose) {
                printf("%s (%lldB) ", p_src_fname, (long long int)comp_size);
                fflush(stdout);
            }
            
            uint8_t *out_buf = NBLIdecompress(in_buf, is_rgb, height, width, use_golomb, use_avp, near, crc32);
            
            delete[] in_buf;
            
            if (out_buf == NULL)
                ERROR("decompress %s failed", p_src_fname);
            
            if (p_dst_fname == NULL) {
                static char dst_fname_buffer [8192];
                p_dst_fname = dst_fname_buffer;
                replaceFileSuffix(p_dst_fname, p_src_fname, "png");
            }
            
            if (verbose) {
                printf("-> %s (%dx%d, %s) ", p_dst_fname, width, height, (is_rgb?"RGB":"Gray"));
                fflush(stdout);
            }
            
            if (!force_write && fileExist(p_dst_fname))
                ERROR("%s already exist", p_dst_fname);
            
            bool failed;
            if (matchSuffixIgnoringCase(p_dst_fname, "pnm") || matchSuffixIgnoringCase(p_dst_fname, "ppm") || matchSuffixIgnoringCase(p_dst_fname, "pgm")) {
                failed = writePNMImageFile(p_dst_fname, out_buf, is_rgb, height, width);
            } else if (matchSuffixIgnoringCase(p_dst_fname, "png")) {
                failed = writePNGImageFile(p_dst_fname, out_buf, is_rgb, height, width);
            } else {
                delete[] out_buf;
                ERROR("unsupported output suffix: %s", p_dst_fname);
            }
            
            delete[] out_buf;
            
            if (failed) {
                ERROR("write %s failed", p_dst_fname);
            }
            
            n_decompressed ++;
        }
        
        if (verbose) {
            size_t n_pixels = (size_t)(is_rgb?3:1) * width * height;
            double bpp      = (8.0*comp_size) / (1.0*width*height);
            double time     = (((double)clock()-start_time) / CLOCKS_PER_SEC) + 0.0001;
            double speed    = (0.001*n_pixels) / time;
            printf("   flag=[%d%s%s]", near, use_golomb?"G":"", use_avp?"A":"");
            if (crc32) printf("   CRC=%08x", crc32);
            printf("   BPP=%.5f   %.4f sec   %.0f kB/s\n" , bpp, time, speed);
        }
    }
    
    
    int n_failed = n_file - n_compressed - n_decompressed;
    
    if (n_file > 1) {
        printf("summary:");
        if (n_compressed)   printf("  %d compressed"  , n_compressed);
        if (n_decompressed) printf("  %d decompressed", n_decompressed);
        if (n_failed)       printf("  %d failed"      , n_failed);
        printf("\n");
    }
    
    return n_failed;
}
