#!/bin/sh
cd "$(dirname $0)"

# --------------------------------- Defaults --------------------------------- #
make_full_log=
debug=
optimize=y
objdir=build
outdir=.
cflags=
ldflags=
arch=64

usage() {
cat <<EOF
Usage: $0 [OPTION]
General options:
  --help                prints help
  --make-full-log       prints complete commands during build
Build options:
  --debug				activate debug mode (-g3 -O0 -DFTM_DEBUG)
  --optimize-disable	disable -O2
  --objdir=OBJDIR		directory for all object (default: ./build)
  --outdir=OUTDIR		directory for all output executable (default : ./bin)
  --arch=ARCH              build architecture (default: 64)
Other tweaks:
  CFLAGS=CFLAGS            some more compilation flags
  LDFLAGS=LDFLAGS          some more linker flags
EOF
exit 0
}

for arg ; do case "$arg" in --help|-h) usage ;; esac; done

for arg ; do case "$arg" in
--make-ful-log)		make_full_log=y ;;
--debug)			debug=y ;;
--optimize-disable)	optimize= ;;
--objdir=*)			objdir="${arg#*=}" ;;
--outdir=*)			outdir="${arg#*=}" ;;
--cflags=*)			cflags="${arg#*=}" ;;
--ldflags=*)		ldflags="${arg#*=}" ;;
--arch=*)			arch="${arg#*=}" ;;
*) echo "Unknown option: $arg"; exit 1 ;;
esac; done

make mrproper MAKE_FULL_LOG=y 1>/dev/null 2>/dev/null

exec 3>&1 1>Makefile.cfg
cat <<EOF
#!/usr/bin/make -f
# ---------------------------------------------------------------------------- #
#                        ./configure.sh generated config                       #
# ---------------------------------------------------------------------------- #
MAKE_FULL_LOG	= $make_full_log
DEBUG			:= $debug
OPTIMIZE		:= $optimize
OBJDIR			:= $objdir
OUTDIR			:= $outdir
CMOREFLAGS		:= $cflags
LDMOREFLAGS		:= $ldflags
ARCH			:= $arch
# End of file
EOF
exec 1>&3 3>&-

chmod +x Makefile.cfg
echo "Wrote configuration, you can 'make' now."