#include "StdAfx.h"
#include "ClassSerialPort.h"
#include <Windows.h>
#include <stdio.h>
#include <ctime>
#include <math.h>



#define  RE_WRITE_TIMES			5
#define  READ_LENGHT			256
#define	 DELAY_TIME				1000

ClassSerialPort::ClassSerialPort(void)
{
}


ClassSerialPort::~ClassSerialPort(void)
{
	if (m_hCom != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCom);
	}
}


/************************************************************************/
/*��ʼ������                                                            */
/************************************************************************/
bool ClassSerialPort::InitPort(unsigned int port,unsigned int baud,unsigned int dataBits,unsigned int stopBits)
{
	char szPort[32] = {0};
	sprintf_s(szPort,"COM%d",port);

	/*��ָ���Ĵ���*/
	m_hCom = CreateFileA(szPort,						//�豸����COM1,COM2
						GENERIC_READ | GENERIC_WRITE,	//����ģʽ,���Զ���д
						0,								//����ģʽΪ0����ʾ������
						NULL,							//��ȫģʽһ������ΪNULL
						OPEN_EXISTING,					//�ò�����ʾ�豸������ڣ����򴴽�ʧ��
						0,								//0��ʾΪͬ����FILE_FLAG_OVERLAPPED��ʾΪ�첽IO
						NULL);							//�Դ��ڶ��ԣ��ò�������ΪNULL

	if (m_hCom == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	BOOL bIsSuccess = true;
	//���ô��ڳ�ʱʱ��,����Ϊ0��ʾ��ʹ�ó�ʱ����
	COMMTIMEOUTS CommimeOuts;
	CommimeOuts.ReadIntervalTimeout = 0;
	CommimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommimeOuts.ReadTotalTimeoutConstant = 0;
	CommimeOuts.WriteTotalTimeoutConstant = 0;
	CommimeOuts.WriteTotalTimeoutMultiplier = 0;

	bIsSuccess = SetCommTimeouts(m_hCom,&CommimeOuts);
	if (bIsSuccess == false)
	{
		return false;
	}

	/*���ô��ڵ�״̬*/
	DCB dcb;
	bIsSuccess = GetCommState(m_hCom,&dcb);
	dcb.BaudRate = baud;//������
	dcb.ByteSize = dataBits;//ÿ���ֽڵ�λ����һ���Ϊ8λ
	dcb.Parity = NOPARITY;//����żУ��λ
	dcb.StopBits = stopBits;//ONESTOPBIT;ֹͣλ
	bIsSuccess = SetCommState(m_hCom,&dcb);
	if (bIsSuccess == false)
	{
		return false;
	}

	/*��մ��ڻ�����*/
	//PURGE_TXABORT :�ж�����д�������������أ���ʹд������û�����
	//PURGE_RXABORT :�ж����ж��������������أ���ʹ��������û�����
	//PURGE_TXCLEAR : ������������
	//PURGE_RXCLEAR : ������뻺����
	PurgeComm(m_hCom,PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	return true;
}

/************************************************************************
����write serial port
************************************************************************/
bool ClassSerialPort::WriteData(const char *data,unsigned int dataLength)
{
	BOOL bResult = true;
	DWORD BytesToSend = 0;
	if (m_hCom == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	
	int i = 0;
	for (i=0; i<RE_WRITE_TIMES; i++)
	{
		bResult = WriteFile(m_hCom,data,dataLength,&BytesToSend,NULL);
		if(!bResult)
		{
			PurgeComm(m_hCom,PURGE_RXCLEAR | PURGE_RXABORT);
			return false;
		}
		else if (BytesToSend != dataLength)
		{
			PurgeComm(m_hCom,PURGE_RXCLEAR | PURGE_RXABORT);
			Sleep(DELAY_TIME);
			continue;
		}
		else
		{
			break;
		}
	}
	if (i >= RE_WRITE_TIMES)
	{
		return false;
	}

	return true;
}

/************************************************************************
���д������ݵĶ�ȡ waitTime�ǵȴ���ʱ��
************************************************************************/
bool ClassSerialPort::ReadData(char *readData,int waitTime)
{
	BOOL bResult = true;
	DWORD BytesRead = 0;
	if (m_hCom == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	long currentTime = GetMyCurrentTime();
	long lastTime = 0;
	int diffTime = 0;
	COMSTAT ComStat;
	DWORD err;
	while(1)
	{
		ClearCommError(m_hCom,&err,&ComStat);
		if (ComStat.cbInQue <= 0)//��黺�����Ƿ������ݿɶ�
		{
			Sleep(DELAY_TIME);
			continue;
		}
		bResult = ReadFile(m_hCom,readData,READ_LENGHT,&BytesRead,NULL);
		if (!bResult)
		{
			lastTime = GetMyCurrentTime();
			diffTime = abs(lastTime - currentTime);
			if (diffTime > waitTime)
			{
				PurgeComm(m_hCom,PURGE_TXCLEAR | PURGE_TXABORT);
				return false;
			}
			Sleep(DELAY_TIME);
		}
	}

	return false;
}


bool ClassSerialPort::ClosePort(void)
{
	if (m_hCom != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCom);
	}
	return true;
}


long ClassSerialPort::GetMyCurrentTime(void)
{
	time_t now_time;
	now_time = time(NULL);
	long currentTime = (long)now_time;
	return currentTime;
}
