/**
 * Copyright (C) 2018 Fuzhou Rockchip Electronics Co., Ltd
 * Author: frank <frank.liu@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "face_capture_process.h"
#include "display.h"
#include "serial_com.h"
#include <pthread.h>
static struct face_win g_face_win[FACE_WINDOW_NUM];
#define FACE_WIN_START 0

static const char* g_face_model_path = "/etc/dsp/face/fr.weights.bin";
static serial_frame frame_format = {.sof = {0xaa, 0xbb},
									.lof = {0x00, 0x00},
									.reserve = {0x00, 0x00, 0x00, 0x00},
									.cmd = {0x00, 0x00},
									.checksum = 0,
									.eof = {0xcc, 0xdd}};

static int face_win_deinit(void)
{
    for (int i = 0; i < FACE_WINDOW_NUM; i++) {
        g_face_win[i].id = -1;
        g_face_win[i].state = 0;
        g_face_win[i].pos = -1;
        g_face_win[i].timeout = 0;

        video_ion_free(&g_face_win[i].ion);
    }

    return 0;
}

static int face_win_init(void)
{
    for (int i = 0; i < FACE_WINDOW_NUM; i++) {
        g_face_win[i].ion.fd = -1;
        g_face_win[i].ion.client = -1;
    }

    for (int i = 0; i < FACE_WINDOW_NUM; i++) {
        g_face_win[i].id = -1;
        g_face_win[i].state = 0;
        g_face_win[i].pos = -1;
        g_face_win[i].timeout = 0;

        if (video_ion_alloc(&g_face_win[i].ion, FACE_WIDTH, FACE_HEIGHT)) {
            printf("%s: %d: video_ion_alloc failed.\n", __func__, __LINE__);
            return -1;
        }
    }

    return 0;
}

static void process_raw_data(int src_x, int src_y, int src_width, int src_height,
                             int width, int height, struct act_object* act_obj)
{
    src_x = src_x - src_width / 4;
    src_x = src_x > 0 ? src_x : 0;

    src_y = src_y - src_height / 4;
    src_y = src_y > 0 ? src_y : 0;

    src_width = 2 * src_width;
    src_width = (src_width + 0xf) & ~0xf;
    src_width = src_width > width ? width : src_width;

    src_height = 2 * src_height;
    src_height = (src_height + 0xf) & ~0xf;
    src_height = src_height > height ? height : src_height;

    src_x = (src_x + src_width) > width ? (width - src_width) : src_x;
    src_y = (src_y + src_height) > height ? (height - src_height) : src_y;

    act_obj->act_x = src_x & ~0xf;
    act_obj->act_y = src_y & ~0xf;
    act_obj->act_w = src_width;
    act_obj->act_h = src_height;
}
int savetofile(void* buffer, int size, int id)
{
	char filename[20] = {0};
	int fd = -1;
	int ret = -1;
	sprintf(filename, "/param/%d", id);
	fd = open(filename, O_CREAT | O_RDWR);
	if(fd != -1) {
		ret = write(fd, buffer, size);
		if(ret == -1) {
			printf("write error\n");
		}
	} else {
		printf("open %s failed\n", filename);
	}
	return ret;
}
static int commit_face(struct video_ion* src_ion, int id, bool is_replace)
{
//    if (!is_replace) {
//        int commit_win = 0;
//
//        for (int i = 0; i < FACE_WINDOW_NUM; i++) {
//            if(g_face_win[i].state)
//                g_face_win[i].pos++;
//        }
//
//        for (int i = 0; i < FACE_WINDOW_NUM; i++) {
//            if (g_face_win[i].state == 0 || g_face_win[i].pos > FACE_WINDOW_NUM)
//                commit_win = i;
//        }
//
//        memcpy(g_face_win[commit_win].ion.buffer, src_ion->buffer,
//               FACE_WIDTH * FACE_HEIGHT * 3 / 2);
//        g_face_win[commit_win].state = 1;
//        g_face_win[commit_win].id = id;
//        g_face_win[commit_win].timeout = FACE_WIN_TIMEOUT;
//        g_face_win[commit_win].pos = FACE_WIN_START;
//
//    } else {
//        for (int i = 0; i < FACE_WINDOW_NUM; i++) {
//            if (g_face_win[i].id == id)
//                memcpy(g_face_win[i].ion.buffer, src_ion->buffer,
//                       FACE_WIDTH * FACE_HEIGHT * 3 / 2);
//        }
//    }
//
//    for (int i = 0; i < FACE_WINDOW_NUM; i++) {
//        if (g_face_win[i].state == 1) {
//            display_the_window(g_face_win[i].pos, g_face_win[i].ion.fd, FACE_WIDTH, FACE_HEIGHT,
//                               RGA_FORMAT_YCBCR_420_SP, FACE_WIDTH, FACE_HEIGHT);
//        }
//    }

    return 0;
}

bool FaceCaptureProcess::get_replace_state(float clear, float front, int id)
{
//    float quality_cache = 1.0, quality_cur = 1.0;
//    struct face_jpeg_cache* tmp_cache = NULL;
//
//    for (std::list<face_jpeg_cache*>::iterator it = face_jpeg_cache_list.begin();
//         it != face_jpeg_cache_list.end(); it++) {
//        if ((*it)->id == id) {
//            tmp_cache = (*it);
//            quality_cache = tmp_cache->clear * tmp_cache->front;
//        }
//    }
//
//    quality_cur = clear * front;
//
//    if (quality_cur - quality_cache > FACE_REPLACE_THRESHOLD) {
//        tmp_cache->clear = clear;
//        tmp_cache->front = front;
//
//        return true;
//    } else {
//        return false;
//    }
return true;

}

char sum(char* frame)
{
	int len = (frame[2] << 8) | frame[3];
	int sum = 0;
	for(int i = 0; i < len - 3; i++) {
		sum += frame[i];
	}
	sum += frame[len - 1] + frame[len - 2];
	sum = ~sum + 1;
	return (char)sum;
}

char check(char* frame)
{
	int len = (frame[2] << 8) | frame[3];
	char sum = 0;
	for(int i = 0; i < len; i++) {
		sum += frame[i];
	}
	return sum;
}

int PacketAndSendData(int fd, char* data, int length)
{
	int checksum = 0;
	int ret;
	if(!data) {
		printf("Data is null\n");
		return -1;
	}
	char* sendbuff = (char*)malloc(sizeof(serial_frame) + length + 1);
	if(!sendbuff) {
		printf("Send buffer malloc failed\n");
		return -1;
	}
	frame_format.lof[0] = (sizeof(serial_frame) + length) >> 8 & 0xff;
	frame_format.lof[1] = (sizeof(serial_frame) + length) & 0xff;
	sendbuff[0] = 0x11;
	memcpy(sendbuff + 1, &frame_format, 10);	//sof + lof + reserve + cmd
	memcpy(sendbuff + 1 + 10, data, length);
	memcpy(sendbuff + 1 + 10 + length, &frame_format.checksum, 3); // checksum + eof
	checksum = sum(sendbuff + 1);
	sendbuff[1 + 10 + length] = checksum;
	ret = UART_Send(fd, sendbuff, sizeof(serial_frame) + length + 1);
	if(sendbuff) {
		free(sendbuff);
	}

	return ret;
}

int FaceCaptureProcess::SerialCMDProcess(int fd, void* frame)
{
	rs_face_feature* pFeature = NULL;
	int faceid = 0;
	if(!frame) {
		printf("Serial frame is NULL\n");
		return -1;
	}
	if(check((char*)frame) == 0){
		printf("Check frame OK\n");
	} else {
		printf("Check frame ERROR\n");
		return -1;
	}

	int cmd = ((char*)frame)[9];
	switch (cmd)
	{
		case 0:
			break;
		case 1:	// register pepole
//			pFeature = (rs_face_feature*)malloc(sizeof(rs_face_feature));
//			if(!pFeature) {
//				printf("face feature buffer malloc failed\n");
//				return -1;
//			}
//			memcpy(pFeature->feature, dst_ion.buffer, 512);
//			faceid = rsRecognitionPersonCreate(mFaceRecognition, pFeature);
//			PacketAndSendData(fd, (char*)&faceid, sizeof(faceid));
			registerflag = 1;
			break;
		case 2:	// delete pepole
//			RS_SDK_API int	rsRecognitionPersonDelete(RSHandle handle, int personId);
			break;
		case 3:
//			RS_SDK_API int	rsRecognitionPersonAddFace(RSHandle handle, rs_face_feature *pFeature, int personId);
			break;
		default:
			break;
	}
}

void* FaceCaptureProcess::SerialProcess(void* __this)
{
	char frame[512] = {0};
	int read_status = 0;
	int len = 0;
	int i = 0;
	int distance = 0;
	int same_side = 1;
	FaceCaptureProcess* _this = (FaceCaptureProcess*)__this;
	_this->read_pos = 0;
	_this->write_pos = 0;
	int ring_size = sizeof(_this->ring_buf);
	memset(_this->ring_buf, 0, sizeof(_this->ring_buf));
	
	while(1) {
		memset(_this->rcv_buf, 0, BUFFER_LEN);
		len = UART_Recv(_this->serial_fd, _this->rcv_buf, BUFFER_LEN);
		if(len > 0)
		{
			if(_this->write_pos + len >=  ring_size) {
				memcpy(&(_this->ring_buf[_this->write_pos]), _this->rcv_buf, ring_size - _this->write_pos);
				memcpy(&(_this->ring_buf[0]), _this->rcv_buf + ring_size - _this->write_pos, len + _this->write_pos - ring_size);
				_this->write_pos = len + _this->write_pos - ring_size;
				same_side = 0;
			} else {
				memcpy(&(_this->ring_buf[_this->write_pos]), _this->rcv_buf, len);
				_this->write_pos += len;
			}
			_this->write_pos %= ring_size;
//			for(int j = 0; j < len; j++) {
//				printf(" %02x", _this->rcv_buf[j]);
//			}
//			printf("\n");
//			for(int j = 0; j < len; j++) {
//				printf(" %02x", _this->ring_buf[_this->read_pos++]);
//				_this->read_pos %= ring_size;
//			}
//			printf("\n");
		}
		else
		{
		  printf("NO serial data\n");
		  continue;
		}
		if(_this->write_pos == _this->read_pos) {
			printf("Ring buffer is empty\n");
			continue;
		}
		distance = _this->write_pos - _this->read_pos;
		while(_this->read_pos != _this->write_pos) {
			if(_this->ring_buf[_this->read_pos] == 0xaa && _this->ring_buf[_this->read_pos+1] == 0xbb) { //head
				frame[i++] = _this->ring_buf[_this->read_pos++ ];
				_this->read_pos %= ring_size;
				read_status = 1;
//				printf("%s:%d ------------------------\n", __func__, __LINE__);
				continue;
			}
			if(read_status == 1) {	//body
				frame[i++] = _this->ring_buf[_this->read_pos++];
				_this->read_pos %= ring_size;
			}
			if(frame[i-1] == 0xdd && frame[i-2] == 0xcc) {
				read_status = 0;
				i=0;
				break;
			}
			if(frame[i-1] == 0xcc && _this->ring_buf[_this->read_pos] == 0xdd) {
				frame[i++] = _this->ring_buf[_this->read_pos++];
				_this->read_pos %= ring_size;
				read_status = 0;
				i=0;
				break;
			}
		}
		if(read_status == 1) {
			continue;
		}

		if(read_status == 0) {
			len = (frame[2] << 8) | frame[3];
			printf("complete frame:");
			for(int j = 0; j < len; j++) {
				printf("%02x", frame[j]);
			}
			printf("\n");
		}

		if(read_status == 0) {
			_this->SerialCMDProcess(_this->serial_fd, frame);
		}
	}
}

FaceCaptureProcess::FaceCaptureProcess(struct Video* video)
    : StreamPUBase("FaceCaptureProcess", true, false)
{
	int err;
    rga_fd = rk_rga_open();
    if (rga_fd < 0) {
        FACE_DBG("%s: rga_open faild.\n", __func__);
        abort();
    }

    rga_dst_buf.fd = -1;
    rga_dst_buf.client = -1;

    if (video_ion_alloc(&rga_dst_buf, FACE_WIDTH, FACE_HEIGHT)) {
        printf("%s: video_ion_alloc failed.\n", __func__);
        abort();
    }

    if (face_win_init()) {
        printf("%s: face_win_init failed.\n", __func__);
        abort();
    }

    dsp_fd = open("/dev/dsp", O_RDWR);
    if (dsp_fd < 0) {
        printf("------> opend dsp dev failed.\n");
        abort();
    }

    model_ion.fd = -1;
    model_ion.client = -1;
    if (video_ion_alloc(&model_ion, 32, 1024 * 1024)) {
        printf("%s: %d -----> model_ion alloc failed.\n", __func__, __LINE__);
        abort();
    }

    dst_ion.fd = -1;
    dst_ion.client = -1;
    if (video_ion_alloc(&dst_ion, FACE_WIDTH, FACE_HEIGHT)) {
        printf("%s: %d -----> dst_ion alloc failed.\n", __func__, __LINE__);
        abort();
    }

    if (readsense_initial_face_recognition(model_ion.buffer, g_face_model_path)) {
        printf("%s:------> readsense_initial_face_recognition failed.\n", __func__);
        abort();
    }

	serial_fd = UART_Open(serial_fd, SERIAL_DEV); //打开串口，返回文件描述符
	if(-1 == serial_fd) {
		printf("open uart failed\n");
		abort();
	}
    err = UART_Init(serial_fd, BAUD_RATE, 0, 8, 1, 'N');

    if(-1 == err){
		printf("Uart init error\n");
		abort();
	}
	
	if (pthread_create(&pid, NULL, SerialProcess, (void*)this)) {
        printf("%s:------> SerialProcess thread create failed.\n", __func__);
        abort();
	}
	registerflag = 0;
}

FaceCaptureProcess::~FaceCaptureProcess()
{
    rk_rga_close(rga_fd);
    video_ion_free(&rga_dst_buf);
    video_ion_free(&model_ion);
    video_ion_free(&dst_ion);
    close(dsp_fd);
    face_win_deinit();
}

int maxid = 0;
int framecount = 0;
bool FaceCaptureProcess::processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf)
{
	
    if (inBuf)
        inBuf->incUsedCnt();

    if (inBuf.get() == NULL || outBuf.get() == NULL) {
        FACE_DBG("%s: input buffer NULL.\n",__func__);
        return false;
    }

    struct face_application_data *face_cap = (struct face_application_data*)inBuf->getPrivateData();
	mLicense = face_cap->mLicense;
	mFaceRecognition = face_cap->mFaceRecognition;
    if (face_cap == NULL) {
        FACE_DBG("%s face_cap get pivate error.\n", __func__);
        return false;
    }

    struct face_info result = face_cap->face_result;

    if (result.count < 0)
        return false;
	
	framecount++;
	for(int i = 0; i < result.count; i++) {
		if(result.objects[i].id > maxid) {
			maxid = result.objects[i].id;
			framecount = 0;
		}
	}
    for (int i = 0; i < result.count; i++) {
        bool is_cached = false;
        bool is_replace = false;
        int id = result.objects[i].id;

        FACE_DBG("Face %d info: id(%d), x_y(%d, %d), w_h(%d, %d), blur_font(%f, %f).\n", i, id,
                 result.objects[i].x, result.objects[i].y,
                 result.objects[i].width, result.objects[i].height,
                 result.blur_prob, result.front_prob);

        /*
         * The actual size is 2 times the original width and height,
         * and ensure the position is within the display.
         */
        struct act_object act_obj;
        process_raw_data(result.objects[i].x, result.objects[i].y,
                         result.objects[i].width, result.objects[i].height,
                         (int)inBuf->getWidth(), (int)inBuf->getHeight(), &act_obj);

        FACE_DBG("act area(%d, %d), w_h(%d, %d).\n",
                 act_obj.act_x, act_obj.act_y, act_obj.act_w, act_obj.act_h);

        struct face_jpeg_cache* tmp_jpeg_cache = NULL;
        for (std::list<face_jpeg_cache*>::iterator it = face_jpeg_cache_list.begin();
             it != face_jpeg_cache_list.end(); it++) {
            if ((*it)->id == id) {
                is_cached = true;
                tmp_jpeg_cache = (*it);
                break;
            }
        }

        float clear = 1 - result.blur_prob;
        float front = result.front_prob;

        if (is_cached)
            is_replace = get_replace_state(clear, front, id);

        if ((!is_cached || is_replace || registerflag) && framecount == 0) {
            int ret;
            int src_fd, src_w, src_h, dst_fd, dst_w, dst_h;
            int src_x_offset, src_y_offset, src_act_w, src_act_h, src_fmt, dst_fmt;

            FACE_DBG("%s: start cut image...\n", __func__);

            src_fd = inBuf->getFd();
            src_w = inBuf->getWidth();
            src_h = inBuf->getHeight();
            src_fmt = RGA_FORMAT_YCBCR_420_SP;
            src_act_w = act_obj.act_w;
            src_act_h = act_obj.act_h;
            src_x_offset = act_obj.act_x;
            src_y_offset = act_obj.act_y;

            dst_fd = rga_dst_buf.fd;
            dst_w = FACE_WIDTH;
            dst_h = FACE_HEIGHT;
            dst_fmt = RGA_FORMAT_YCBCR_420_SP;

//            ret = rk_rga_ionfd_to_ionfd_rotate_offset_ext(rga_fd, src_fd,
//                                                          src_w, src_h, src_fmt,
//                                                          src_w, src_h, src_act_w, src_act_h,
//                                                          src_x_offset, src_y_offset,
//                                                          dst_fd, dst_w, dst_h, dst_fmt, 0);
            if (ret)
                FACE_DBG("%s: %d rga fail!\n", __func__, __LINE__);
			void* src_phy = inBuf->getPhyAddr();
			void* src_vir = inBuf->getVirtAddr();
//            ret = readsense_face_recognition(model_ion.buffer, (void*)model_ion.phys, NULL, NULL,
//            								dst_ion.buffer, (void*)dst_ion.phys, rga_dst_buf.buffer,
//                                             (void*)rga_dst_buf.phys, dsp_fd, 1);
//			float landmarks21[21*2] = {1164, 496,
//									 1196, 466,
//									 1239, 473,
//									 1275, 470,
//									 1318, 459,
//									 1352, 485,
//									 1188, 516,
//									 1208, 512,
//									 1229, 513,
//									 1290, 509,
//									 1311, 505,
//									 1331, 507,
//									 1261, 556,
//									 1237, 578,
//									 1262, 584,
//									 1286, 575,
//									 1214, 625,
//									 1262, 609,
//									 1310, 620,
//									 1263, 645,
//									 1263, 630};
//			 
//			 int fd = open("/param/test_1080p.bin", O_RDONLY);
//			 int filelen = 0;
//			 void* filebuff = NULL;
//			 if(fd != -1) {
//			 	filelen = lseek(fd,0L,SEEK_END);
//				lseek(fd,0L,SEEK_SET);
//			 	filebuff = malloc(filelen);
//				if(filebuff) {
//					ret = read(fd, (char*)filebuff, filelen);
//				}
//			 }
			 ret = readsense_face_recognition(model_ion.buffer, (void*)model_ion.phys, src_vir, src_phy, 
							 dst_ion.buffer, (void*)dst_ion.phys, rga_dst_buf.buffer, (void*)rga_dst_buf.phys,
							 dsp_fd, src_w, src_h, 
							 1, (float *)&result.objects[i].landmarks21);
//			 ret = readsense_face_recognition(model_ion.buffer, (void*)model_ion.phys, filebuff, filebuff, 
//							 dst_ion.buffer, (void*)dst_ion.phys, rga_dst_buf.buffer, (void*)rga_dst_buf.phys,
//							 dsp_fd, 1920, 1080, 
//							 1, landmarks21);

//            if (ret)
//                printf("-------> readsense_face_recognition failed.\n");
//			else
//				printf("-------> readsense_face_recognition successful.\n");
            commit_face(&rga_dst_buf, id, is_replace);
//			float *feature = (float*)dst_ion.buffer;
//			printf("readsense_face_feature:");
//			for(int i = 0; i < 512; i++) {
//				if(i %16 == 0) {
//					printf("\n");
//				}
//				printf("%f\t", feature[i]);
//			}
//			printf("\n");

			rs_face_feature feature;
			feature.version = 0;
			int faceid = 0;
			memcpy(feature.feature, dst_ion.buffer, 512*sizeof(float));
			if(registerflag) {
				faceid = rsRecognitionPersonCreate(mFaceRecognition, &feature);
				frame_format.cmd[0] = 0x00;
				frame_format.cmd[1] = 0x01;
				PacketAndSendData(serial_fd, (char*)&faceid, sizeof(faceid));
				registerflag = 0;
//				savetofile(rga_dst_buf.buffer, FACE_WIDTH * FACE_HEIGHT * 3 / 2, faceid);
				printf("-------> Person create successful. person id is %d \n", faceid);
			} else {
				result_of_recog rs_result;
				faceid = rsRecognitionFaceIdentification(mFaceRecognition, &feature);
				frame_format.cmd[0] = 0x00;
				frame_format.cmd[1] = 0x02;
				rs_result.faceid = faceid;
				rs_result.confidence = rsRecognitionGetConfidence(mFaceRecognition);
				PacketAndSendData(serial_fd, (char*)&rs_result, sizeof(rs_result));
				printf("size of data %d\n", sizeof(rs_result));
				printf("-------> Recognition Face successful. person id is %d confidence:%f\n", faceid, rs_result.confidence);
			}
	
            /* add tmp_jpeg_cache to face_jpeg_cache_list */
            if (!is_cached) {
                tmp_jpeg_cache = new face_jpeg_cache;
                tmp_jpeg_cache->id = id;
                tmp_jpeg_cache->width = act_obj.act_w;
                tmp_jpeg_cache->height = act_obj.act_h;
                tmp_jpeg_cache->clear = clear;
                tmp_jpeg_cache->front = front;
                face_jpeg_cache_list.push_back(tmp_jpeg_cache);
            }
        }

        for (int i = 0; i < FACE_WINDOW_NUM; i++) {
            if (g_face_win[i].state == 1 && id == g_face_win[i].id)
                g_face_win[i].timeout = FACE_WIN_TIMEOUT;
        }
    }

    for (int i = 0; i < FACE_WINDOW_NUM; i++) {
        if (g_face_win[i].state == 1 && (--g_face_win[i].timeout) == 0) {
            shield_the_window(g_face_win[i].pos, false);
            g_face_win[i].state = 0;
        }
    }

    for (std::list<face_jpeg_cache*>::iterator it = face_jpeg_cache_list.begin();
         it != face_jpeg_cache_list.end();) {
        bool find = false;
        for (int i = 0; i < result.count; i++) {
            if ((*it)->id == result.objects[i].id) {
                find = true;
                break;
            }
        }

        if (!find) {
            delete (*it);
            face_jpeg_cache_list.erase(it++);
        } else {
            it++;
        }
    }

    if (inBuf)
        inBuf->decUsedCnt();

    return true;
}
