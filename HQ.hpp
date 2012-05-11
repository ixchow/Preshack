#ifndef HQ_HPP
#define HQ_HPP

extern unsigned int hq_bits; //bits of precision display is assumed to have.

void init_hq(unsigned int samples);
bool have_hq();
void start_hq_frame();
void finish_hq_frame();
void deinit_hq();

#endif //HQ_HPP
