#ifndef __STORAGE_H__
#define __STORAGE_H__

#define STORAGE_NAMESPACE           "NVS_DATA"
#define CONFIG_KEY                  "config"
#define CREDENTIALS_KEY             "credentials"

extern size_t credentials_length;

int mount_storage(void);
int write_nvs(char *key, char *value, int size);
int read_nvs(char *key, char *value, size_t *size);
int erase_nvs(void);
int erase_key_nvs(char *key);

#endif
