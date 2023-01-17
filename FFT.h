#ifndef __FFT_H
#define __FFT_H

#include "arm_math.h"
#include "arm_const_structs.h"

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define pi 3.1416f
#define NPT_max 4096    //最大FFT点数
#define adc_bit 12      //ADC位数
#define adc_max 3.3f    //ADC的满量程，单位V

/*
 *FFT通道结构体的建立
 *adc_buf： ADC采样进来的数据
 *input：   fft运算的输入数组
 *Amp：     幅值，单位V
 *phase：   相位，弧度制
 *phase_f： 基频相位，弧度制
 *f：       频率，单位Hz
 *vp：      基频幅值，单位V
 *dc：      直流分量，单位V
 */
typedef struct FFTchannel
{
    u32     adc_buf[NPT_max];   //ADC采样进来的数据
    float   input[NPT_max * 2]; //fft运算的输入数组
    float   Amp[NPT_max];       //幅值，单位V
    float   phase[NPT_max];     //相位，弧度制
    float   phase_f;            //基频相位，弧度制
    float   f;                  //频率，单位Hz
    float   vp;                 //基频幅值，单位V
    float   dc;                 //直流分量，单位V
}FFTchannel;

typedef enum WinType//定义窗的种类
{
    None = 0,//不加窗，即矩形窗
    Hanning_win,//汉宁窗（能处理大部分的问题）
    Hamming_win,//汉明窗
    Flattop_win,//平顶窗（算基频幅值很准）
    Kaiser_win//凯泽窗（算THD很准）
}WinType;

/*
 *对通道采样的数据进行FFT
 *ch：通道的名称
 *NPT：FFT点数
 *FS：采样率
 *win_type：加窗的类型
 *支持的窗的类型：None, Hanning_win, Hamming_win, Flattop_win, Kaiser_win
 *
 */
void UserFFT(FFTchannel* ch, int NPT, float Fs, WinType win_type);

//以下为各种窗函数的生成函数
int Factorial(int num);//阶乘
float Besseli(float x);//0阶贝塞尔
float Kaiser(float beta, int n, float* win);//Kaiser窗
float FlattopWin(int n, float* win);//Flattop窗
float Hanning(int n, float* win);//Hanning窗
float Hamming(int n, float* win);//Hamming窗
float rectangle(int n, float* win);//矩形窗
void Window(float* input, float* win, int n);//加窗函数

#endif
