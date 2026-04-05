#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

#define STBIDEF inline

STBIDEF unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels) { return 0; }
STBIDEF void stbi_image_free(void *retval_from_stbi_load) {}

#endif
