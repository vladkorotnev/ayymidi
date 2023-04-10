#ifndef DISP_H_
#define DISP_H_

#define DISP_UPD_INTERVAL 16 //ms
#define DISP_MS2TICKS(m) (m/DISP_UPD_INTERVAL)

void disp_begin();
void disp_midi_light();
void disp_ch_swap(bool);
void disp_tick();
void disp_show_msg(uint16_t time, const char * top_line, const char * bottom_line);
void disp_stop_msg();

#endif