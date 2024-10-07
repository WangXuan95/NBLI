#ifndef   __FILE_IO_H__
#define   __FILE_IO_H__

#include <cstdint>
#include <cstdio>


inline static bool fileExist (const char *p_filename) {
    FILE *fp = fopen(p_filename, "rb");
    if (fp) fclose(fp);
    return (fp != NULL);
}


// return:   false : success
//            true : failed
inline static bool writeBytesToFile (const char *p_filename, uint8_t *p_buf, size_t len) {
    FILE *fp;
    
    if ((fp = fopen(p_filename, "wb")) == NULL)
        return true;
    
    if (len != fwrite(p_buf, sizeof(uint8_t), len, fp)) {
        fclose(fp);
        return true;
    }
    
    fclose(fp);
    return false;
}


// return:  NULL     : failed
//          non-NULL : pointer to data, need to be delete later
inline static uint8_t* loadBytesFromFile (const char *p_filename, size_t &len) {
    FILE *fp;
    
    if ((fp = fopen(p_filename, "rb")) == NULL)
        return NULL;
    
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    uint8_t *p_buf = new uint8_t[len + (1<<9)];
    
    if (p_buf)
        fread(p_buf, sizeof(uint8_t), len, fp);
    
    fclose(fp);
    return p_buf;
}

#endif // __FILE_IO_H__
