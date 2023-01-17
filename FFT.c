#include "FFT.h"
#include "Kaiser.h"
#include "stdio.h"
#include "math.h"

float pDst[NPT_max];        //暂存FFT后的幅值
float win[NPT_max];         //窗函数
float win_input[NPT_max];   //做窗处理的数据

void UserFFT(FFTchannel* ch, int NPT, float Fs, WinType win_type)
{
    u32 sum = 0;
    float avg = 0;
    for (int i = 0; i < NPT; i++)
    {
        sum += ch->adc_buf[i];
    }

    avg = sum * 1.0f / NPT;//算出直流分量
    ch->dc = avg / 4096 * 3.3f;

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

    for (int i = 0; i < NPT; i++)
	{
		ch->phase[i] = atan2f(ch->input[2 * i + 1], ch->input[2 * i]);
	}//计算相位

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

    ch->phase_f = ch->phase[x_max];
    ch->f = Fs / NPT * x_max;
    ch->vp = max / pow(2, adc_bit) * adc_max * k;//还要乘恢复系数k

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
