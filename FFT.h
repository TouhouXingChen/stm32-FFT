#ifndef __FFT_H
#define __FFT_H

#include "arm_math.h"
#include "arm_const_structs.h"

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define pi 3.1416f
#define NPT_max 1024    //最大FFT点数

/*
 *FFT通道结构体的建立
 *adc_buf： ADC采样进来的数据
 *input：   fft运算的输入数组
 *Amp：     幅值，单位V
 *phase_f： 基频相位，弧度制
 *f：       频率，单位Hz
 *vp：      基频幅值，单位V
 *dc：      直流分量，单位V
 *THD:      谐波失真度(%)
 */
typedef struct FFTchannel
{
    u32     adc_buf[NPT_max];       //ADC采样进来的数据
    float   input[NPT_max * 2];     //fft运算的输入数组
    float   Amp[NPT_max];           //幅值，单位V
    float   phase_f;                //基频相位，弧度制
    float   f;                      //频率，单位Hz
    float   vp;                     //基频幅值，单位V
    float   dc;                     //直流分量，单位V
    float   THD_array[10];          //计算THD的数组，最多计算10次谐波
    float   THD;                    //谐波失真度
    float   ifft[NPT_max * 2];      //进行逆傅里叶变换的数组，是FFT(&ch, 'f')函数的频谱（复数）
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
 *FFT初始化函数
 *NPT_init：FFT点数（地址传参）
 *Fs_init：采样率（地址传参）
 *adc_bit_init：ADC位数，12位就传入12
 *adc_max_init：ADC满量程（单位V）
 *THD_max_init：计算THD（谐波失真度）时的谐波次数，最大是10
 *注：
 *计算THD时的谐波次数不仅与你设置的这个THD_max有关，还与你设置的采样率有关
 *要保证FFT后的频谱中能找到那个谐波。采样率过低可能就找不到高次谐波
 *找不到高次谐波的话，我的处理方案就是只计算能采到的谐波，不会导致出错，但肯定有误差
 */
void FFT_Init(int* NPT_init, float* Fs_init, int adc_bit_init, float adc_max_init, int THD_max_init);

/*
 *设置FFT运算的窗函数
 *f_win_set：计算基频频率的窗函数
 *vp_win_set：计算基频幅值的窗函数
 *thd_win_set：计算THD（谐波失真度）的窗函数
 *支持的窗函数类型：None, Hanning_win, Hamming_win, Flattop_win, Kaiser_win
 */
void SetWindow(WinType f_win_set, WinType vp_win_set, WinType thd_win_set);

/*
 *用户的FFT函数
 *参数只需要传入做FFT的通道就行
 *如果想要加窗，通过SetWindow()函数来设置窗函数
 *默认窗函数是矩形窗（不加窗）
 */
void UserFFT(FFTchannel* ch);

/*
 *注：这个函数不需要被调用，已经在UserFFT中调用
 *计算FFT的主体函数
 *ch：计算FFT的通道
 *FFT_type：本次FFT是计算哪个参数的
 *'f'：本次FFT算的是基频频率
 *'v'：本次FFT算的是基频幅值
 *'t'：本次FFT算的是THD（谐波失真度）
 */
void FFT(FFTchannel* ch, char FFT_type);

/*
 *用户的逆傅里叶变换函数
 *进行逆傅里叶变换的是通道中的ifft数组
 */
void UserIFFT(FFTchannel* ch, float* out);

//以下为各种窗函数的生成函数
int Factorial(int num);//阶乘
float Besseli(float x);//0阶贝塞尔
float Kaiser(float beta, int n, float* win_out);//Kaiser窗
float FlattopWin(int n, float* win_out);//Flattop窗
float Hanning(int n, float* win_out);//Hanning窗
float Hamming(int n, float* win_out);//Hamming窗
float rectangle(int n, float* win_out);//矩形窗
void Window(float* input, float* win_in, int n);//加窗函数

#endif
