// Включения
#include "CStreamInOut.h"

/**
   Конструктор класса
   на входе    :  *
*/
CStreamOut::CStreamOut()
{
   Stop();
}

/**
   Деструктор класса
*/
CStreamOut::~CStreamOut()
{
}

/**
   Запуск потока
   на входе    :  in_Address     - адрес
                  in_u8StreamID  - идентификатор потока
                  in_pBuffer     - указатель на буфер с данными
                  in_stSize      - размер данных
   на выходе   :  *
*/
void CStreamOut::Start(iridium_address_t in_Address, u8 in_u8StreamID, u8* in_pBuffer, size_t in_stSize)
{
   m_Address = in_Address;
   m_u8StreamID = in_u8StreamID;
   m_u8BlockID = 0;
   m_u32SendTime = 0;
   m_pBuffer = in_pBuffer;
   m_pEnd = m_pBuffer + in_stSize;
}

/**
   Остановка потока
   на входе    :  *
   на выходе   :  *
*/
void CStreamOut::Stop()
{
   m_Address = 0;
   m_u8StreamID = 0;
   m_u8BlockID = 0;
   m_u32SendTime = 0;
   m_pBuffer = NULL;
   m_pEnd = NULL;
}

/**
   Установка буфера
   на входе    :  in_pBuffer  - указатель на буфер
                  in_stSize   - размер данных
   на выходе   :  *
*//*
void CStreamOut::SetBuffer(u8* in_pBuffer, size_t in_stSize)
{
   m_pBuffer = in_pBuffer;
   m_pEnd = m_pBuffer + in_stSize;
}*/
      
/**
   Пропуск данных
   на входе    :  in_stSize   - количество пропускаемых данных
   на выходе   :  *
*/
void CStreamOut::Skip(size_t in_stSize)
{
   // Сдвиг буфера
   m_pBuffer += in_stSize;
   
   // Проверка на выход за пределы
   if(m_pBuffer >= m_pEnd)
      m_pBuffer = m_pEnd;

   m_u8BlockID++;
   m_u32SendTime = 0;
}

/**
   Обработка данных
   на входе    :  *
   на выходе   :  *
*/
void CStreamOut::Work()
{
   bool l_bClose = false;
   // Проверка режима отправки данных
   if(m_pBuffer)
   {
      // Проверим время отправки сообщения
      if(m_u32SendTime)
      {
         // Проверка максимального времени ожидания
         if((GetCurrentTime() - m_u32SendTime) > MAX_STREAM_WAIT_TIME)
            l_bClose = true;

      } else
      {
         // Получение размера оставшихся данных
         size_t l_stSize = GetSize();
         if(l_stSize)
         {
            // Если размер блока больше максимального
            if(l_stSize > MAX_STREAM_BLOCK_SIZE)
               l_stSize = MAX_STREAM_BLOCK_SIZE;

            //AddToLog("Send block address: %x stream: %d block: %d size: %d ptr: %x", l_Address, l_u8StreamID, l_u8Block, l_u16Size, l_pPtr);
            StreamBlock(m_Address, m_u8StreamID, m_u8BlockID, l_stSize, m_pBuffer);

            // Установка времени отправки
            m_u32SendTime = GetCurrentTime();

         } else
            l_bClose = true;
      }
      // Проверка на закрытие потока
      if(l_bClose)
      {
         // Отправка сообщения по закрытию потока
         StreamClose(m_Address, m_u8StreamID);
         // Очистка потока
         Stop();
      }
   }
}
