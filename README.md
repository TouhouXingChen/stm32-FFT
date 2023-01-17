[TOC]

#stm32 基于CubeMX的DSP库FFT函数封装
##一、序言

&emsp;&emsp;**由于在测量题中FFT的应用十分广泛，每次调库的过程都十分繁琐。为了避免重复操作，实现功能更多，调库更方便，我重构了调用DSP库FFT的代码。本次重构更稳定地实现了更多的功能。**

&emsp;&emsp;*需要注意的是，本次封装只支持官方DSP库的单片机。例程中使用的是stm32F407ZGT6。*
##二、说明
###1.功能说明
* 支持的FFT点数有：16位，32位，64位，128位，256位，512位，1024位，2048位，4096位。
* 支持多通道的FFT。
* 能通过对ADC采样数据的FFT运算得出以下数据：
    * 各频率的幅值（单位：V）
    * 基频幅值（单位V）
    * 基频大小（单位：Hz）
    * 各频率的相位（弧度制）
    * 直流分量（单位V）
* ***可以在运行中途改变FFT的采样率、FFT点数。当频率范围很宽时，可以直接调整FFT点数。***
###2.定义说明
* FFT通道结构体
```c
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
```
* 窗的种类
```c
typedef enum WinType//定义窗的种类
{
    None = 0,//不加窗，即矩形窗
    Hanning_win,//汉宁窗（能处理大部分的问题）
    Hamming_win,//汉明窗
    Flattop_win,//平顶窗（算基频幅值很准）
    Kaiser_win//凯泽窗（算THD很准）
}WinType;
```
* 封装完成的FFT函数
```c
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
```
* 其他定义
```c
#define adc_bit 12      //ADC位数
#define adc_max 3.3f    //ADC的满量程，单位V
```
&emsp;*这两个值根据实际情况改正即可*
###3.文件说明
&emsp;**本次封装一共包含三个文件**
* FFT.h
  *包含通道结构体的定义，窗类型的定义。*
* FFT.c
  *封装了FFT函数，加窗函数。*
* Kaiser.h
  *封装了1024点，2048点，4096点的凯泽窗。凯泽窗的运算复杂度较高，用stm32生成很慢，所以我提前生成好了。*
##三、调用步骤
1. 在Keil中添加路径和文件
   * 魔法棒中添加宏：
    >       ARM_MATH_CM4,ARM_MATH_MATRIX_CHECK,ARM_MATH_ROUNDING,__FPU_PRESENT=1U
   * 魔法棒中添加路径：
    >       ..\Drivers\CMSIS\DSP\Include
   * 左侧文件栏中Drivers/CMSIS中添加文件：
    >       ..\Drivers\CMSIS\Lib\arm_cortexM4lf_math.lib
2. 引用头文件
   ```c
   #include "FFT.h"
   ```
3. 定义一个通道
   ```c
   FFTchannel ch1//定义了一个FFT的通道
   ```
4. 开启ADC采样
   ```c
   //开启ADC采样
   //ch1.adc_buf即为DMA搬运的存储数组
    HAL_ADC_Start_DMA(&hadc1, ch1.adc_buf, NPT);
   ```
5. FFT运算
   ```c
   //将需要FFT的通道，FFT点数，采样率，加窗类型都传入
   //这里的NPT不是宏定义，是可以根据需求来改变的变量
    UserFFT(&ch1, NPT, Fs, Flattop_win);
   ```
6. 将结果打印
   ```c
    send[0] = ch1.f;//基频频率(Hz)
    send[1] = ch1.vp;//基频幅值(V)
    send[2] = ch1.dc;//直流分量(V)
    My_printf(&huart1, "f:{.1}Hz, vp:{.3}V, dc:{.3}V{e}", send);//串口，我自己定义的，不必在意
   ```
##四、补充
若有两个通道要进行FFT，例如要进行求相位差等操作时，实例化两个结构体即可。
```c
#include "FFT.h"

FFTchannel ch1, ch2;
...//省去FFT的过程
phase_dif = ch1.phase_f - ch2.phase_f;
```
像AD7606这种的，实例化8个通道就行了。是有点多，但不麻烦
##五、最后想说的
&emsp;&emsp;**我写这个FFT函数的封装的目的是为了造福大伙，更方便地使用FFT。对于这个封装还有任何建议的，也欢迎指出，我以后也有可能会更新一些内容。更详细的应用可以到例程中去看，我初步用我的stm32F407检验起来，是没有大问题的。**

作者：汪家铖
