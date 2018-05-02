1、	使用说明
（1）	开机后，串口连接PC，PC串口调试工具输入video运行video程序；
（2）	按VOL+键切换录像模式和回放模式；
（3）	录像模式下，按UPDATE键UI会显示菜单，通过LEFF键、RIGHT键、UP键、DOWN键来选择具体菜单功能，按VOL-键确认选择该菜单；
（4）	录像模式下，UI不显示菜单的情况下，按UP键开始录像，再次按UP键结束录像。
（5）	录像模式下，UI不显示菜单的情况下，按VOL-键一次拍照一次。
（6）	回放模式下，通过LEFT键和RIGHT键选择文件，JPG图片文件直接显示预览，MP4文件需要按VOL-键确认播放，按RETURN键退出播放。
（7）	接多个摄像头的情况下，在录像模式下，UI不显示菜单的情况下，通过按RETURN键来切换不同摄像头预览。
2、	文件说明
（1）	alsa_capture.c、alsa_capture.h :声音采样程序。
（2）	camera_ui.c、camera_ui_def.h、camera_ui.h、camera_ui_res_cn.h、camera_ui_res_en.h、camera_ui_res_tw.h :MiniGUI相关程序
（3）	common.h：公共程序，包含相关结构体定义
（4）	cvr_make.sh：编译脚本
（5）	encoder_muxing：MP4 mux相关程序
（6）	huffman.h：Huffman表
（7）	Makefile
（8）	number.h：水印数字表
（9）	parameter.c、parameter.h：参数配置程序
（10）	res：MiniGUI相关资源
（11）	rga_copybit.h、rga_yuv_scal.c、rga_yuv_scal.h：rga相关程序
（12）	settime.c、settime.h：时间设置程序
（13）	ueventmonitor：uevent事件程序
（14）	video.cpp、video.h：录像、显示、拍照程序
（15）	video_ion_alloc.c、video_ion_alloc.h：ion申请程序
（16）	videoplay.c、videoplay.h：视频、图片回放程序
（17）	video_rkfb.c、video_rkfb.h：fb操作相关函数
（18）	vpu.c、vpu.h：图片编码、解码相关函数
（19）	wifi_management.c、wifi_management.h：wifi相关程序
