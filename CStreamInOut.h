#ifndef _C_STREAM_IN_OUT_H_INCLUDED_
#define _C_STREAM_IN_OUT_H_INCLUDED_

#include "Iridium.h"

#define MAX_STREAM_BLOCK_SIZE 240
#define MAX_STREAM_WAIT_TIME  12000

class CStreamOut
{
public:
   // Конструктор/деструктор
   CStreamOut();
   virtual ~CStreamOut();

   // Запуск/остановка потока
   void Start(iridium_address_t in_Address, u8 in_u8StreamID, u8* in_pBuffer, size_t in_stSize);
   void Stop();

   // Установка/получение идентификатора потока
   void SetStreamID(u8 in_u8StreamID)
      { m_u8StreamID = in_u8StreamID; }
   u8 GetStreamID()
      { return m_u8StreamID; }

   // Установка/получение идентификатора блока
   void SetBlockID(u8 in_u8BlockID)
      { m_u8BlockID = in_u8BlockID; }
   u8 GetBlockID()
      { return m_u8BlockID; }

   void Skip(size_t in_stSize);

   u8* GetData()
      { return m_pBuffer; }
   size_t GetSize()
      { return m_pEnd - m_pBuffer; }

   void Work();

   //////////////////////////////////////////////////////////////////////////
   // Перегруженные методы
   //////////////////////////////////////////////////////////////////////////
   virtual u32 GetCurrentTime()
      { return 0; }
   // Отправка блока данных
   virtual void StreamBlock(iridium_address_t in_Address, u8 in_u8StreamID, u8 in_u8Block, u16 in_u16Size, void* in_pPtr)
      { }
   // Закрытие потока
   virtual void StreamClose(iridium_address_t in_Address, u8 in_u8StreamID)
      { }

protected:
   iridium_address_t m_Address;                    // Адрес запросившего поток
   u8                m_u8StreamID;                 // Идентификатор потока
   u8                m_u8BlockID;                  // Идентификатор блока
   u32               m_u32SendTime;                // Время отправки данных
   u8*               m_pBuffer;                    // Текущий указатель на данные 
   u8*               m_pEnd;                       // Указатель на конец буфера
};

#endif   // _C_STREAM_IN_OUT_H_INCLUDED_
