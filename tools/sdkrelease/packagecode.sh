#!/bin/sh

CURR_DIR=$(pwd)
BOOTPATH="svn://192.168.18.90/repository/code/TR6260/Boot"
BOOTVERSION=5185
BOOTDIFFPATH="svn://192.168.18.90/repository/code/TR6260/boot_diff"
BOOTDIFFVERSION=5162
TRUNKPATH="svn://192.168.18.90/repository/code/TR6260/tr6260"
RELEASEPATH="svn://192.168.18.90/repository/tags/TR6260/release"
DOCPATH="svn://192.168.18.90/repository/code/TR6260/Doc"
TOOLSPATH="svn://192.168.18.90/repository/code/TR6260/Tools"
NVPATH="svn://192.168.18.90/repository/code/TR6260/NV"

# debug mode, will printout log on console
# TAGDEBUG="on"
# test mode, code will not commit to svn except tag method
# TAGTEST="on"

METHOD="$1"
TAGPATH="$2"

TARGETTMPDIR=$CURR_DIR/tr6260_tag
TARGETCONFIG=$CURR_DIR/packageconfig
COMPILELOG=$CURR_DIR/compileError.log

if [ $# -ne 2 ] ;then
	echo "useage: sh tag/sdk/release tagpath"
	exit
fi

TRUNKTOTAG ()
{
	tt_trunkpath="$1"
	tt_tagpath="$2"

	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "tt_trunkpath=$tt_trunkpath"
		echo "tt_tagpath=$tt_tagpath"
	fi

	#boot version check
	#boot can not compile in linux enviroment
	#svn last changed version to check if the boot.bin is newest

	bootversion=`svn info $BOOTPATH | grep "Rev:" | awk '{print $4}'`
	bootdiffversion=`svn info $BOOTDIFFPATH | grep "Rev:" | awk '{print $4}'`

	echo "bootversion check"
	if [ $bootversion -ne $BOOTVERSION -o $bootdiffversion -ne $BOOTDIFFVERSION ] ;then
		echo "boot not the lastest version."
		echo "bootversion=$bootversion lastbootversion=$BOOTVERSION bootdiffversion=$bootdiffversion lastbootdiffversion=$BOOTDIFFVERSION"
		return
	fi
	echo "bootversion check success..."

	echo "create $tt_tagpath dirlist"
	if [ "X$TAGTEST" = "X" ] ;then
		svn info $tt_tagpath > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			echo "$tt_tagpath already create"
			return
		fi
		
		svn mkdir --parents $tt_tagpath/code -m "create $METHOD path" > /dev/null 2>&1
		svn mkdir --parents $tt_tagpath/bin -m "create $METHOD path" > /dev/null 2>&1
	fi
	echo "create $tt_tagpath success"

	if [ "X$TAGTEST" = "X" ] ;then
		svn cp $tt_trunkpath $tt_tagpath/code -m "add trunk code" > /dev/null 2>&1
		svn cp $BOOTPATH $tt_tagpath/code -m "add boot code" > /dev/null 2>&1
		svn cp $BOOTDIFFPATH $tt_tagpath/code -m "add boot diff code" > /dev/null 2>&1
		svn cp $NVPATH $tt_tagpath -m "add nv code" > /dev/null 2>&1
		svn cp $DOCPATH $tt_tagpath -m "add doc code" > /dev/null 2>&1
		svn cp $TOOLSPATH $tt_tagpath -m "add tools code" > /dev/null 2>&1
	fi

	echo "Add tag success ..."
}

if [ "X$METHOD" = "Xtag" ] ;then
	rm -rf $TARGETTMPDIR
	mkdir $TARGETTMPDIR; cd $TARGETTMPDIR
	TRUNKTOTAG "$TRUNKPATH" "$TAGPATH"
	rm -rf $TARGETTMPDIR
	exit
fi

LOCALCOMPILE ()
{
	lc_parenetdir=$(pwd)
	lc_codepath="$1"
	lc_build="$2"
	lc_param="$3"

	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "lc_parenetdir=$lc_parenetdir"
		echo "lc_codepath=$lc_codepath"
		echo "lc_build=$lc_build"
		echo "lc_param=$lc_param"
	fi

	cd $lc_codepath/code/tr6260/scripts
	if [ $? -ne 0 ] ;then
		echo "please check the code path"
		exit
	fi
	echo "#####################################" > $COMPILELOG
	echo "$lc_build $lc_param" >> $COMPILELOG
	echo "#####################################" >> $COMPILELOG

	echo "compile $lc_build $lc_param"
	chmod u+x $lc_build
	./$lc_build $lc_param >> $COMPILELOG 2>&1

	if grep -q Error $COMPILELOG ; then
		echo "compile error. please check the $COMPILELOG"
		exit
	else
		echo "compile $lc_build success..."
		rm -rf $COMPILELOG
	fi

	cd $lc_parenetdir
}

RMBUILDFILE ()
{
	rb_parenetdir=$(pwd)
	rb_filepath="$1"
	rb_filename="$2"
	
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "rb_parenetdir=$rb_parenetdir"
		echo "rb_filepath=$rb_filepath"
		echo "rb_filename=$rb_filename"
	fi

	cd $rb_filepath
	
	rb_filelist=$(ls | grep build)
	for rb_file in $rb_filelist
	do
		if [ "X$rb_file" != "X$rb_filename" ] ;then
			rm -rf $rb_file
		fi
	done

	cd $rb_parenetdir
}

RMMKFILE ()
{
	rm_parenetdir=$(pwd)
	rm_filepath="$1"
	rm_filename="$2"
	
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "rm_parenetdir=$rm_parenetdir"
		echo "rm_filepath=$rm_filepath"
		echo "rm_filename=$rm_filename"
	fi

	cd $rm_filepath
	rm_filelist=$(ls | grep "Makefile.")
	#echo $rm_filelist

	for rm_file in $rm_filelist
	do
		if [ "X$rm_file" != "X$rm_filename" ] ;then
			rm -rf $rm_file
		fi
	done

	cd $rm_parenetdir
}

RMWITHALLFILE ()
{
	ra_parenetdir=$(pwd)
	ra_filepath="$1"
	ra_filename="$2"
	
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "ra_parenetdir=$ra_parenetdir"
		echo "ra_filepath=$ra_filepath"
		echo "ra_filename=$ra_filename"
	fi

	cd $ra_filepath

	ra_filelist=$(ls)
	#echo $ra_filelist
	for ra_file in $ra_filelist
	do
		if [ "X$ra_file" != "X$ra_filename" ] ;then
			rm -rf $ra_file
		fi
	done

	cd $rm_parenetdir
}

RMPARTITIONFILE ()
{
	rp_parenetdir=$(pwd)
	rp_filepath="$1"
	rp_filename="$2"
	
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "rp_parenetdir=$rp_parenetdir"
		echo "rp_filepath=$rp_filepath"
		echo "rp_filename=$rp_filename"
	fi

	cd $rp_filepath;rm -rf *.csv

	rp_filelist=$(ls | grep partition)
	#echo $rp_filelist
	for rp_file in $rp_filelist
	do
		if [ "X$rp_file" != "X$rp_filename" ] ;then
			rm -rf $rp_file
		fi
	done

	cd $rp_parenetdir
}

LOCALFILECHANGE ()
{
	lc_parenetdir=$(pwd)
	lc_sdkdir="$1"
	lc_build="$2"
	lc_refdesign="$3"
	lc_mkfile="$4"
	lc_partcfg="$5"
	lc_appcfg="$6"

	if [ "X$TAGDEBUG" != "X" ] ;then	
		echo "lc_parenetdir=$lc_parenetdir"
		echo "lc_sdkdir=$lc_sdkdir"
		echo "lc_build=$lc_build"
		echo "lc_refdesign=$lc_refdesign"
		echo "lc_mkfile=$lc_mkfile"
		echo "lc_partcfg=$lc_partcfg"
		echo "lc_appcfg=$lc_appcfg"
	fi

	cd $lc_sdkdir
	find . -type d -name ".svn" | xargs rm -rf;
	rm -rf Tools
	rm -rf code/tr6260/wifi/hal
	rm -rf code/tr6260/wifi/mac
	rm -rf code/tr6260/platform/psm
	echo $lc_build | grep diff
	if [ $? -eq 0 ] ;then
		cp code/boot_diff/out/fullmask/boot_nocrc.bin bin/
	else
		cp code/Boot/out/fullmask/boot_nocrc.bin bin/
	fi
	rm -rf code/boot_diff
	rm -rf code/Boot

	RMBUILDFILE "code/tr6260/scripts" "$lc_build"
	RMMKFILE "code/tr6260/scripts" "$lc_mkfile"
	RMWITHALLFILE "code/tr6260/apps" "$lc_appcfg"
	RMWITHALLFILE "code/tr6260/ref_design" "$lc_refdesign"
	RMPARTITIONFILE "NV" "$lc_partcfg"

	if grep -q DUSE_WIFI_LIB code/tr6260/scripts/$lc_mkfile ; then
		sed -i 's/#DEFINE += -DUSE_WIFI_LIB/DEFINE += -DUSE_WIFI_LIB/g' code/tr6260/scripts/$lc_mkfile > /dev/null
	fi
	if grep -q DUSE_WIFI_LIB code/tr6260/scripts/$lc_mkfile ; then
		sed -i 's/#DEFINE += -DUSE_PSM_LIB/DEFINE += -DUSE_PSM_LIB/g' code/tr6260/scripts/$lc_mkfile > /dev/null
	fi
	
	cd $lc_parenetdir
}

LOCALSDKHANDLE ()
{
	ld_parenetdir=$(pwd)
	ld_tagpath="$1"
	ld_sdkdir="$2"
	ld_refdesign="$3"
	ld_mkfile="$4"
	ld_partcfg="$5"
	ld_appcfg="$6"
	ld_build="$7"
	ld_param="$8"

	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "ld_parenetdir=$ld_parenetdir"
		echo "ld_tagpath=$ld_tagpath"
		echo "ld_sdkdir=$ld_sdkdir"
		echo "ld_refdesign=$ld_refdesign"
		echo "ld_build=$ld_build"
		echo "ld_param=$ld_param"
		echo "ld_mkfile=$ld_mkfile"
		echo "ld_partcfg=$ld_partcfg"
		echo "ld_appcfg=$ld_appcfg"
	fi
	mkdir -p $ld_sdkdir; cd $ld_sdkdir
	mkdir bin
	echo "download... wait minutes"
	svn co $ld_tagpath ld_srccode > /dev/null 2>&1
	if [ $? -ne 0 ] ; then
		echo "can not download code from $ld_tagpath"
		return
	fi
	LOCALCOMPILE "ld_srccode" "$ld_build" "$ld_param"
	echo "download... wait minutes"
	svn co $ld_tagpath ld_dstcode > /dev/null 2>&1
	if [ $? -ne 0 ] ; then
		echo "can not download code from $ld_tagpath"
		return
	fi
	cp ld_srccode/code/tr6260/lib/libwifi.a  ld_dstcode/code/tr6260/lib/ > /dev/null 2>&1
	cp ld_srccode/code/tr6260/lib/liblmacwifi.a ld_dstcode/code/tr6260/lib/ > /dev/null 2>&1
	cp ld_srccode/code/tr6260/lib/libpsm.a ld_dstcode/code/tr6260/lib/ > /dev/null 2>&1
	LOCALFILECHANGE "ld_dstcode" "$ld_build" "$ld_refdesign" "$ld_mkfile" "$ld_partcfg" "$ld_appcfg"
	cp ld_dstcode/* ./ -rf
	LOCALCOMPILE "ld_dstcode" "$ld_build" "$ld_param"

	ld_binlist=`find ld_dstcode/code/tr6260/out -name *.bin`
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "ld_binlist=$ld_binlist"
	fi
	for ld_bin in $ld_binlist
	do
		echo $ld_bin | grep 0x004000 > /dev/null 2>&1
		if [ $? -eq 0 ] ;then
			cp $ld_bin bin/
		fi

		echo $ld_bin | grep 0x007000 > /dev/null 2>&1
		if [ $? -eq 0 ] ;then
			cp $ld_bin bin/
		fi

		echo $ld_bin | grep ota > /dev/null 2>&1
		if [ $? -eq 0 ] ;then
			cp $ld_bin bin/
		fi
	done

	rm -rf ld_srccode
	rm -rf ld_dstcode

	cd $ld_parenetdir
}

SVNADDLIB ()
{
	sl_parenetdir=$(pwd)
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "sl_parenetdir=$sl_parenetdir"
	fi
	
	sl_filelist=`find . -name *.a && find . -name *.o`
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo $sl_filelist
	fi
	for sl_file in $sl_filelist
	do
		svn add $sl_file > /dev/null 2>&1
	done
}

TAGTOSDK ()
{
	ts_parenetdir=$(pwd)
	ts_tagpath="$1"
	ts_sdkpath="$2"
	
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "ts_parenetdir=$ts_parenetdir"
		echo "ts_tagpath=$ts_tagpath"
		echo "ts_sdkpath=$ts_sdkpath"
	fi
	mkdir ts_sdkcode; cd ts_sdkcode

	while read line
	do
		ts_refdesign=`echo $line | cut -d : -f1`
		ts_build=`echo $line | cut -d : -f2`
		ts_mkfile=`echo $line | cut -d : -f3`
		ts_partcfg=`echo $line | cut -d : -f4`
		ts_appcfg=`echo $line | cut -d : -f5`

		if [ "X$ts_refdesign" = "X" -o "X$ts_build" = "X" -o "X$ts_mkfile" = "X" -o "X$ts_partcfg" = "X" ] ;then
			echo "config may something error"
			exit
		fi

		if [ -z $ts_appcfg ] ;then
			ts_appcfg="noapp"
		fi

		ts_param=`echo $ts_build | awk '{print $2}'`
		if [ "X$ts_param" != "X" ] ;then
			ts_build=`echo $ts_build | awk '{print $1}'`
		fi

		ts_sdkdir=`echo $ts_build | cut -d _ -f2- | cut -d . -f1`
		ts_pathdir=${ts_sdkpath##*/}
		ts_sdkdir="$ts_pathdir""_$ts_sdkdir"

		if [ X$TAGDEBUG != X ] ;then
			echo "ts_tagpath=$ts_tagpath"
			echo "ts_sdkdir=$ts_sdkdir"
			echo "ts_refdesign=$ts_refdesign"
			echo "ts_mkfile=$ts_mkfile"
			echo "ts_partcfg=$ts_partcfg"
			echo "ts_appcfg=$ts_appcfg"
			echo "ts_build=$ts_build"
			echo "ts_param=$ts_param"
		fi
		LOCALSDKHANDLE "$ts_tagpath" "$ts_sdkdir" "$ts_refdesign" "$ts_mkfile" "$ts_partcfg" "$ts_appcfg" "$ts_build" "$ts_param"
	done < $TARGETCONFIG

	cd $ts_parenetdir > /dev/null 2>&1

	if [ "X$TAGTEST" = "X" ] ;then
		svn mkdir $ts_sdkpath -m "create sdk dir" > /dev/null 2>&1
		if [ $? -ne 0 ] ; then
			echo "can not create $ts_sdkpath"
			return
		fi
		svn co $ts_sdkpath ts_commit > /dev/null 2>&1
		cp ts_sdkcode/* ts_commit/ -rf
		mkdir ts_commit/Tools
		svn co $TOOLSPATH ts_tools > /dev/null 2>&1
		cp ts_tools/* ts_commit/Tools/ -rf

		cd ts_commit; svn add * > /dev/null 2>&1
		SVNADDLIB
		svn ci -m "add sdk code" > /dev/null 2>&1
	fi
	echo "add sdk success..."
	cd $ts_parenetdir
}

if [ "X$METHOD" = "Xsdk" ] ;then
	rm -rf $TARGETTMPDIR;rm -rf $COMPILELOG
	mkdir $TARGETTMPDIR;cd $TARGETTMPDIR;
	TAGTOSDK "$TAGPATH" "$TAGPATH""_SDK"
	if [ "X$TAGDEBUG" = "X" ] ;then
		rm -rf $TARGETTMPDIR
	fi
	exit
fi

SDKTORELEASE ()
{
	sr_parenetdir=$(pwd)
	sr_sdkpath="$1"
	sr_releasepath="$2"

	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "sr_parenetdir=$sr_parenetdir"
		echo "sr_sdkpath=$sr_sdkpath"
		echo "sr_releasepath=$sr_releasepath"
	fi
	echo "download sdk file, wait ..."
	svn co $sr_sdkpath sdkcode > /dev/null 2>&1
	
	if [ $? -ne 0 ] ;then
		echo "can not download code. please check the tag path"
		exit
	fi
	cd sdkcode
	find . -type d -name ".svn" | xargs rm -rf;

	sr_filelist=$(ls | grep TR6260)
	if [ "X$TAGDEBUG" != "X" ] ;then
		echo "sr_filelist=$sr_filelist"
	fi

	for sr_file in $sr_filelist
	do
		mkdir $sr_file/Tools
		cp Tools/* $sr_file/Tools/ -r
		ls $sr_file/code/tr6260/scripts | grep build | grep diff
		if [ $? -ne 0 ] ;then
			rm -rf $sr_file/Tools/diff
		fi
		zip -r "$sr_file"".zip" $sr_file > /dev/null 2>&1
		rm -rf $sr_file
	done

	rm -rf Tools
	cd $sr_parenetdir

	sdkpathdir=${sr_sdkpath##*/}
	if [ "X$TAGTEST" = "X" ] ;then
		if [ "X$TAGDEBUG" != "X" ] ;then
			echo "$sr_releasepath"/"$sdkpathdir"
		fi
		svn mkdir --parents "$sr_releasepath"/"$sdkpathdir" -m "create release dir" > /dev/null 2>&1
		if [ $? -ne 0 ] ; then
			echo "can not create "$sr_releasepath"/"$sdkpathdir""
			return
		fi
		svn co "$sr_releasepath"/"$sdkpathdir" releasecode > /dev/null 2>&1
		cp sdkcode/* releasecode/ -r
		cd releasecode
		if [ "X$TAGDEBUG" != "X" ] ;then
			echo $(ls)
		fi
		svn add * > /dev/null 2>&1
		svn ci -m "$sdkpathdir release版本发布" > /dev/null 2>&1
	fi
	cd $sr_parenetdir
	
	echo "Add release success"
}

if [ "X$METHOD" = "Xrelease" ] ;then
	rm -rf $TARGETTMPDIR;mkdir $TARGETTMPDIR;cd $TARGETTMPDIR
	SDKTORELEASE "$TAGPATH" "$RELEASEPATH"
	if [ "X$TAGDEBUG" = "X" ] ;then
		rm -rf $TARGETTMPDIR
	fi
	exit
fi
