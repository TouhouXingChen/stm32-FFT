#include "FFT.h"
#include "Kaiser.h"
#include "stdio.h"
#include "math.h"

float pDst[NPT_max];        //暂存FFT后的幅值
float win[NPT_max];         //窗函数
float win_input[NPT_max];   //做窗处理的数据

int* pNPT = NULL;           //FFT点数
float* pFs = NULL;          //采样率

WinType f_win = None;       //计算基频频率时加的窗函数
WinType vp_win = None;      //计算基频幅值时加的窗函数
WinType thd_win = None;     //计算THD（谐波失真度）时加的窗函数

int adc_bit = 12;           //ADC位数
float adc_max = 3.3f;       //ADC的满量程，单位V
int THD_max = 5;            //计算谐波失真度时的谐波次数

void FFT_Init(int* NPT_init, float* Fs_init, int adc_bit_init, float adc_max_init, int THD_max_init)
{
    pNPT = NPT_init;
    pFs = Fs_init;
    adc_bit = adc_bit_init;
    adc_max = adc_max_init;
    if (THD_max_init > 10)
    {
        THD_max = 10;
    }
    else
    {
        THD_max = THD_max_init;
    }
}

void SetWindow(WinType f_win_set, WinType vp_win_set, WinType thd_win_set)
{
    f_win = f_win_set;
    vp_win = vp_win_set;
    thd_win = thd_win_set;
}

void UserFFT(FFTchannel* ch)
{
    int NPT;
    if (pNPT == NULL)
    {
        NPT = 1024;
    }
    else
    {
        NPT = *pNPT;
    }
    //分三次FFT，每个FFT为了更好地实现测量，加的窗的类型不同。使用SetWindow()函数设置
    FFT(ch, 'f');//计算频率的FFT
    for (int i = 0; i < NPT * 2; i++)
    {
        ch->ifft[i] = ch->input[i];
    }
    FFT(ch, 'v');//计算基频幅值的FFT
    FFT(ch, 't');//计算THD的FFT
}

void FFT(FFTchannel* ch, char FFT_type)
{
    int NPT;
    float Fs;
    WinType win_type = None;

    if (pNPT == NULL || pFs == NULL)
    {
        //为了防止忘记初始化导致的错误，这里设置一个初值
        NPT = 1024;
        Fs = 0;
    }
    else
    {
        //导入FFT点数和采样率
        NPT = *pNPT;
        Fs = *pFs;
    }

    switch(FFT_type)//设置加的窗函数
    {
    case 'f':
        win_type = f_win;
        break;
    case 'v':
        win_type = vp_win;
        break;
    case 't':
        win_type = thd_win;
        break;
    default:
        win_type = None;
        break;
    }

    u32 sum = 0;
    float avg = 0;
    for (int i = 0; i < NPT; i++)
    {
        sum += ch->adc_buf[i];
    }

    avg = sum * 1.0f / NPT;//算出直流分量
    ch->dc = avg / pow(2, adc_bit) * adc_max;

    for (int i = 0; i < NPT; i++)
    {
        win_input[i] = ch->adc_buf[i] - avg;//减去直流分量
    }

    float k = 0;//窗的恢复系数
    switch(win_type)
    {
    case None:
        k = rectangle(NPT, win);
        Window(win_input, win, NPT);//加窗
        break;
    case Hanning_win:
        k = Hanning(NPT, win);
        Window(win_input, win, NPT);//加窗
        break;
    case Hamming_win:
        k = Hamming(NPT, win);
        Window(win_input, win, NPT);//加窗
        break;
    case Flattop_win:
        k = FlattopWin(NPT, win);
        Window(win_input, win, NPT);//加窗
        break;
    case Kaiser_win:
        switch(NPT)
		{
			case 1024:
				Window(win_input, Kaiser_1024, NPT);
				break;
			case 2048:
				Window(win_input, Kaiser_2048, NPT);
				break;
			case 4096:
				Window(win_input, Kaiser_4096, NPT);
				break;
		}
        k = 1;
        break;
    }

    for (int i = 0; i < NPT; i++)
    {
        ch->input[2 * i] = win_input[i];
        ch->input[2 * i + 1] = 0;
    }//对输入数组进行预处理，数组下标是偶数的存实部，数组下标是奇数的存虚部

    switch(NPT)
    {
    case 16:
        arm_cfft_f32(&arm_cfft_sR_f32_len16, ch->input, 0, 1);
        break;
    case 32:
        arm_cfft_f32(&arm_cfft_sR_f32_len32, ch->input, 0, 1);
        break;
    case 64:
        arm_cfft_f32(&arm_cfft_sR_f32_len64, ch->input, 0, 1);
        break;
    case 128:
        arm_cfft_f32(&arm_cfft_sR_f32_len128, ch->input, 0, 1);
        break;
    case 256:
        arm_cfft_f32(&arm_cfft_sR_f32_len256, ch->input, 0, 1);
        break;
    case 512:
        arm_cfft_f32(&arm_cfft_sR_f32_len512, ch->input, 0, 1);
        break;
    case 1024:
        arm_cfft_f32(&arm_cfft_sR_f32_len1024, ch->input, 0, 1);
        break;
    case 2048:
        arm_cfft_f32(&arm_cfft_sR_f32_len2048, ch->input, 0, 1);
        break;
    case 4096:
        arm_cfft_f32(&arm_cfft_sR_f32_len4096, ch->input, 0, 1);
        break;
    }//根据FFT点数的不同进行FFT运算

    // for (int i = 0; i < NPT; i++)
	// {
	// 	ch->phase[i] = atan2f(ch->input[2 * i + 1], ch->input[2 * i]);
	// }//计算相位

    arm_cmplx_mag_f32(ch->input, pDst, NPT);//计算幅值
    for (int i = 0; i < NPT; i++)
    {
        if (i == 0)
        {
            ch->Amp[i] = pDst[i] / NPT;
        }
        else
        {
            ch->Amp[i] = pDst[i] * 2 / NPT;
        }
    }//计算出最终的幅值

    float max = ch->Amp[1];
    int x_max = 1;
    for (int i = 2; i < NPT / 2; i++)
    {
        if (ch->Amp[i] > max)
        {
            max = ch->Amp[i];
            x_max = i;
        }
    }//找出幅值最大的点的横坐标

    int thd_point = thd_point = NPT / 2 / x_max;//决定计算THD的时候取几次谐波
    if (thd_point > THD_max)
    {
        thd_point = THD_max;//防止数组越界
    }

    switch(FFT_type)
    {
    case 'f'://计算基频和基频相位
        ch->phase_f = atan2f(ch->input[2 * x_max + 1], ch->input[2 * x_max]);
        ch->f = Fs / NPT * x_max;
        break;
    case 'v'://计算基频幅值
        ch->vp = max / pow(2, adc_bit) * adc_max * k;//还要乘恢复系数k
        break;
    case 't'://计算THD
        ch->THD_array[0] = max;
        float thd_sum = 0;
        
        for (int i = 2; i <= thd_point; i++)
        {
            float max2 = 0;
            for (int j = -2; j <= 2; j++)
            {
                if (ch->Amp[i * x_max + j] > max2)
                {
                    max2 = ch->Amp[i * x_max + j];
                }
            }
            ch->THD_array[i - 1] = max2;
        }
        for (int i = 1; i < thd_point; i++)
        {
            thd_sum += ch->THD_array[i] * ch->THD_array[i];
        }
        ch->THD = sqrt(thd_sum) / ch->THD_array[0] * 100;
        break;
    }
}

void UserIFFT(FFTchannel* ch, float* out)
{
    int NPT;
    if (pNPT == NULL)
    {
        NPT = 1024;
    }
    else
    {
        NPT = *pNPT;
    }
    for (int i = 0; i < NPT; i++)
    {
        ch->ifft[2 * i + 1] = -ch->ifft[2 * i + 1];
    }
    switch(NPT)
    {
    case 16:
        arm_cfft_f32(&arm_cfft_sR_f32_len16, ch->ifft, 0, 1);
        break;
    case 32:
        arm_cfft_f32(&arm_cfft_sR_f32_len32, ch->ifft, 0, 1);
        break;
    case 64:
        arm_cfft_f32(&arm_cfft_sR_f32_len64, ch->ifft, 0, 1);
        break;
    case 128:
        arm_cfft_f32(&arm_cfft_sR_f32_len128, ch->ifft, 0, 1);
        break;
    case 256:
        arm_cfft_f32(&arm_cfft_sR_f32_len256, ch->ifft, 0, 1);
        break;
    case 512:
        arm_cfft_f32(&arm_cfft_sR_f32_len512, ch->ifft, 0, 1);
        break;
    case 1024:
        arm_cfft_f32(&arm_cfft_sR_f32_len1024, ch->ifft, 0, 1);
        break;
    case 2048:
        arm_cfft_f32(&arm_cfft_sR_f32_len2048, ch->ifft, 0, 1);
        break;
    case 4096:
        arm_cfft_f32(&arm_cfft_sR_f32_len4096, ch->ifft, 0, 1);
        break;
    }
    for (int i = 0; i < NPT; i++)
    {
        out[i] = ch->ifft[i * 2] * 1.0f / NPT + ch->dc / pow(2, adc_bit) * adc_max;
    }
}

/*-------阶乘-------*/
int Factorial(int num)
{
	int a = 1;
	for (int i = num; i >= 1; i--)
	{
		a *= i;
	}
	return a;
}
/*--------0阶贝塞尔--------*/
float Besseli(float x)
{
	float a = 1.0;
	for (int i = 1; i < 20; i++)
	{
		a += pow((pow((x / 2), i) * 1.0 / Factorial(i)), 2);
	}
	return a;
}
/*--------Kaiser窗--------*/
float Kaiser(float beta, int n, float* win_out)
{
	for (int i = 0; i < n; i++)
	{
		win_out[i] = Besseli(beta * sqrt(1 - pow(2.0 * i / (n - 1) - 1, 2))) / Besseli(beta);
	}
	return 1;
}
/*--------Flattop窗--------*/
float FlattopWin(int n, float* win_out)
{
	float a0 = 0.21557895, a1 = 0.41663158, a2 = 0.277263158, a3 = 0.083578947, a4 = 0.006947368;
	for (int i = 0; i < n; i++)
	{
		win_out[i] = a0 - a1 * cos(2 * pi * i / (n - 1)) + a2 * cos(4 * pi * i / (n - 1))
			- a3 * cos(6 * pi * i / (n - 1)) + a4 * cos(8 * pi * i / (n - 1));
	}
	return 4.639;
}
/*--------Hanning--------*/
float Hanning(int n, float* win_out)
{
	for (int i = 0; i < n; i++)
	{
		win_out[i] = 0.5 * (1 - cos(2 * pi * i / (n - 1)));
	}
	return 2;
}
/*--------Hamming--------*/
float Hamming(int n, float* win_out)
{
	for (int i = 0; i < n; i++)
	{
		win_out[i] = 0.54 - 0.46 * cos(2 * pi * i / (n - 1));
	}
	return 1.852;
}
/*--------rectangle--------*/
float rectangle(int n, float* win_out)
{
	for (int i = 0; i < n; i++)
	{
		win_out[i] = 1;
	}
	return 1;
}
/*--------加窗--------*/
void Window(float* input, float* win_in, int n)
{
	for (int i = 0; i < n; i++)
	{
		input[i] *= win_in[i];
	}
}
