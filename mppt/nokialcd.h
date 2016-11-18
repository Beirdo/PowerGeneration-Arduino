#ifndef NOKIALCD_H__
#define NOKIALCD_H__

#ifdef __CPLUSPLUS
extern "C" {
#endif

#define LCD_X     84
#define LCD_Y     48

#define COL_COUNT (LCD_X / 7)
#define ROW_COUNT (LCD_Y / 8)

typedef char screen_line_t[COL_COUNT];
extern screen_line_t screen_lines[ROW_COUNT];
extern char show_temperature;
extern int swap_count;

void LcdInitialize(void);
void ScreenInitialize(void);
void ScreenRefresh(void);

#ifdef __CPLUSPLUS
}
#endif

#endif
