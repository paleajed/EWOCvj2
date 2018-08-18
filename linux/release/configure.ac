#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([EWOCvj2], [2.00], [paleajed@gmail.com])
AC_CONFIG_SRCDIR([../../src/start.cpp])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([openal], [alGenBuffers], [], [
              echo "The OpenAL Soft library is required for this program. (Audio support)."
              exit -1])
AC_CHECK_LIB([rtmidi], [main], [], [
              echo "The RtMidi library is required for this program. (Audio support)."
              exit -1])
AC_CHECK_LIB([avcodec], [av_codec_next], [], [
              echo "The FFMPEG avcodec library with HAP support (decoder+encoder) is required for this program."
              exit -1])
AC_CHECK_LIB([avdevice], [avdevice_version], [], [
              echo "The FFMPEG avdevice library is required for this program."
              exit -1])
AC_CHECK_LIB([avformat], [av_get_packet], [], [
              echo "The FFMPEG avformat library is required for this program."
              exit -1])
AC_CHECK_LIB([avutil], [avutil_version], [], [
              echo "The FFMPEG avutil library is required for this program."
              exit -1])
AC_CHECK_LIB([swscale], [swscale_version], [], [
              echo "The FFMPEG swscale library is required for this program."
              exit -1])
AC_CHECK_LIB([glut], [glutInit], [], [
              echo "The freeglut library is required for this program."
              exit -1])
AC_CHECK_LIB([freetype], [FT_Init_FreeType], [], [
              echo "The freetype2 library is required for this program."
              exit -1])
AC_CHECK_LIB([X11], [XGetWindowAttributes], [], [
              echo "The X11 library is required for this program."
              exit -1])
AC_CHECK_LIB([SDL2], [SDL_GetMouseFocus], [], [
              echo "The SDL2 library is required for this program."
              exit -1])
AC_CHECK_LIB([GL], [glClear], [], [
              echo "The OpenGL library is required for this program."
              exit -1])
AC_CHECK_LIB([GLEW], [glewInit], [], [
              echo "The glew library is required for this program."
              exit -1])
AC_CHECK_LIB([jpeg], [jpeg_start_compress], [], [
              echo "The libjpeg library is required for this program."
              exit -1])
AC_CHECK_LIB([snappy], [snappy_uncompress], [], [
              echo "The snappy library is required for this program."
              exit -1])
AC_CHECK_LIB([boost_system], [main], [], [
              echo "The boost system library is required for this program."
              exit -1])
AC_CHECK_LIB([boost_filesystem], [rename], [], [
              echo "The boost filesystem library is required for this program."
              exit -1])
AC_CHECK_LIB([glib-2.0], [g_free], [], [
              echo "The glib library is required for this program."
              exit -1])
AC_CHECK_LIB([gobject-2.0], [g_free], [], [
              echo "The gobject library is required for this program."
              exit -1])
AC_CHECK_LIB([gtk-3], [gtk_init_check], [], [
              echo "The GTK3+ library is required for this program."
              exit -1])
AC_CHECK_LIB([nfd], [NFD_PickFolder], [], [
              echo "The native file dialog (NFD) library is required for this program."
              exit -1])
AC_CHECK_LIB([lo], [lo_address_new], [], [
              echo "The liblo library >= 0.29 is required for this program. (OSC support)."
              exit -1])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([malloc.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset pow sqrt])

AC_CONFIG_FILES([Makefile])
AM_INIT_AUTOMAKE([foreign])
AC_OUTPUT
