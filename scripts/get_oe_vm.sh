# https://gitee.com/openeuler/RISC-V/blob/master/doc/tutorials/vm-qemu-oErv.md#%E5%8F%AF%E9%80%89-%E5%87%86%E5%A4%87-qemu-x86_64-%E6%88%96-arm64-%E7%8E%AF%E5%A2%83

# TODO move to config directory (along with clangd config)

#MIRROR_URL=https://mirror.iscas.ac.cn/openeuler
MIRROR_URL=https://mirror.sjtu.edu.cn/openeuler

BASE_DIR=$(realpath $(dirname ${BASH_SOURCE[0]})/..)
! test -d $BASE_DIR/images && echo "Images directory not found, is this a your VM directory?" && exit 1
! test -d $BASE_DIR/scripts && echo "Scripts directory not found, is this a your VM directory?" && exit 1

arch=$(uname -m)

# version selection
echo "Available versions:"
curl $MIRROR_URL/ 2> /dev/null | grep -E "href.*openEuler" | while read line
do
	# this only works for certain mirror
	# remove prefix and suffix
	str=${line#*href=\"\.\/}
	str=${str%%/*}
	# echo "$line --> $str"
	printf '\t%s\n' $str
done

echo ""
read -p "Please choose your version: " version

if ! [[ $version == "openEuler-*" ]]
then
	version=openEuler-$version
	echo "Compact version detected. Expanded to $version."
fi

image_dir=$BASE_DIR/images/$version
boot_script=$BASE_DIR/scripts/start-$version.sh

# download
echo Download files to $image_dir, please wait.

test -f $image_dir && echo "Target directory $image_dir exists. Abort" && exit 1
mkdir -p $image_dir
cd $image_dir

! wget $MIRROR_URL/$version/virtual_machine_img/$arch/$version-$arch.qcow2.xz && \
	echo qcow2.xz download failed. && exit 1
echo qcow2.xz downloaded.

echo Decompression started in background.
xz -d $version-$arch.qcow2.xz && echo Decompression completed. &
wget $MIRROR_URL/$version/OS/$arch/images/pxeboot/initrd.img && echo initrd downloaded.
wget $MIRROR_URL/$version/OS/$arch/images/pxeboot/vmlinuz && echo vmlinuz downloaded.

# template script
test -f $boot_script || echo "Writing template boot script to $boot_script"
test -f $boot_script || cat >> $boot_script << EOF
#!/usr/bin/bash
time_str=\$(date +%Y%m%d-%H%M%S)

cd $image_dir

qemu-system-$arch				\\
	-nographic				\\
	-enable-kvm				\\
	-smp 8 -m 16G				\\
	-kernel vmlinuz				\\
	-initrd initrd.img			\\
	-hda $version-$arch.qcow2		\\
	-net user,hostfwd=::5555-:22 -net nic	\\
	-append 'root=/dev/sda2 console=ttyS0'	\\
	-D $BASE_DIR/logs/$version-\$time_str
EOF
test -f $boot_script && chmod +x $boot_script

echo "Waiting for background jobs..."
wait
echo "Setup completed."
