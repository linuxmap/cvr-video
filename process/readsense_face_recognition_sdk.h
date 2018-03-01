#ifndef READSENSE_FACE_RECOGNITION_SDK_H
#define READSENSE_FACE_RECOGNITION_SDK_H

#define DSP_ALGORITHM_FACE_RECOGNITION    0xa0000004

#define FACE_WIDTH 128
#define FACE_HEIGHT 128

#define FACE_RECOGNITION_FEATURE_DIMENSION 512

int readsense_initial_face_recognition(void * model_virt, const char * weights_path);

int readsense_face_recognition(void * model_virt, void * model_phys, void * raw_virt, void * raw_phys, 
				void * dst_virt, void * dst_phys, void * internal_virt, void * internal_phys,
				int dsp_fd, int raw_width, int raw_height, 
				int face_num, float * landmark_position);

float get_fr_feature_similarity(const float * fr1, const float * fr2, const int dim);

#endif
