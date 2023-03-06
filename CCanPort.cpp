#include "CCANPort.h"

/**
   Конструктор класса
   на входе    :  *
   на выходе   :  *
*/
CCANPort::CCANPort()
{
   m_u8Address = 0;
   m_u8TID = 0;
   m_u16CANID = 0;
   memset(&m_InBuffer, 0, sizeof(m_InBuffer));
   memset(&m_OutBuffer, 0, sizeof(m_OutBuffer));
   m_u32PacketCANID = 0;
   memset(&m_aPacketBuffer, 0, sizeof(m_aPacketBuffer));
   m_stPacketSize = 0;
   m_bTransmite = false;
   m_pAccess = NULL;
}

/**
   Деструктор класса
*/
CCANPort::~CCANPort()
{
}

/**
   Установка входящего буфера
   на входе    :  in_pBuffer  - указатель на буфер
                  in_stSize   - размер буфера
   на выходе   :  успешность установки буфера
*/
bool CCANPort::SetInBuffer(void* in_pBuffer, size_t in_stSize)
{
   bool l_bResult = false;
   
   if(in_pBuffer && in_stSize > sizeof(can_frame_t))
   {
      // Вычисление максимального колличества CAN кадров в буфере
      m_InBuffer.m_stMax = in_stSize / sizeof(can_frame_t);
      m_InBuffer.m_pBuffer = (can_frame_t*)in_pBuffer;
      // Очистка буфера
      memset(in_pBuffer, 0, in_stSize);
      l_bResult = true;
   }
   return l_bResult;
}

/**
   Установка входящего буфера
   на входе    :  in_pBuffer  - указатель на буфер
                  in_stSize   - размер буфера
   на выходе   :  успешность установки буфера
*/
bool CCANPort::SetOutBuffer(void* in_pBuffer, size_t in_stSize)
{
   bool l_bResult = false;
   
   if(in_pBuffer && in_stSize > sizeof(can_frame_t))
   {
      // Вычисление максимального количества CAN кадров в буфере
      m_OutBuffer.m_stMax = in_stSize / sizeof(can_frame_t);
      m_OutBuffer.m_pBuffer = (can_frame_t*)in_pBuffer;
      // Очистка буфера
      memset(in_pBuffer, 0, in_stSize);
      l_bResult = true;
   }
   return l_bResult;
}

/**
   Добавление CAN кадра в буфер
   на входе    :  in_pFrame   - указатель на данные CAN кадра
   на выходе   :  успешность добавления CAN кадра
*/
bool CCANPort::AddFrame(can_frame_t* in_pFrame)
{
   bool l_bResult = false;

   if(m_InBuffer.m_stCount)
   {
      // Освобождение "зависших" пакетов, обход с конца списка
      u16 l_u16NewID = (in_pFrame->m_u32ExtID >> IRIDIUM_EXT_ID_CAN_ID_SHIFT) & IRIDIUM_EXT_ID_CAN_ID_MASK;
      u8 l_u8NewNum = (in_pFrame->m_u32ExtID >> IRIDIUM_EXT_ID_TID_SHIFT) & IRIDIUM_EXT_ID_TID_MASK;

      // Поиск замыкающего CAN кадра
      can_frame_t* l_pEnd = m_InBuffer.m_pBuffer + (m_InBuffer.m_stCount - 1);
      for(can_frame_t* l_pCur = l_pEnd; l_pCur >= m_InBuffer.m_pBuffer; l_pCur--)
      {
         // Проверка что проверяемый кадр от того же источника
         if(l_u16NewID == ((l_pCur->m_u32ExtID >> IRIDIUM_EXT_ID_CAN_ID_SHIFT) & IRIDIUM_EXT_ID_CAN_ID_MASK))
         {
            // Если проверяемый кадр является замыкающим кадром, остановим поиск
            if(l_pCur->m_u32ExtID & IRIDIUM_EXT_ID_END_MASK)                                                                                                     
               break;
            else
            {
               // Если проверяемый кадр имеет другой номер транзакции, отметим кадр к удалению
               if(l_u8NewNum != ((l_pCur->m_u32ExtID >> IRIDIUM_EXT_ID_TID_SHIFT) & IRIDIUM_EXT_ID_TID_MASK))
                  l_pCur->m_u32ExtID = 0;
            }
         }
      }
   }

   // Проверка наличия места в буфере
   if(m_InBuffer.m_stCount < m_InBuffer.m_stMax)
   {
      // Копирование CAN кадра
      memcpy(m_InBuffer.m_pBuffer + m_InBuffer.m_stCount, in_pFrame, sizeof(can_frame_t));
      m_InBuffer.m_stCount++;
      l_bResult = true;
   } else
   {
#if 0
      // Пакет не помещается в буфер
      printf("CCANPort::AddFrame ERROR!!!\r\n");
      Dump();
#endif
      l_bResult = false;
   }

   return l_bResult;
}

/**
   Добавление пакета в буфер (разложение на CAN кадры)
   на входе    :  in_bBroadcast  - признак широковещательного пакета
                  in_u8Address   - адрес на который надо отправлять CAN кадры
                  in_pBuffer     - указатель на буфер с данными
                  in_stSize      - размер данных в буфере
   на выходе   :  успешность добавления данных
*/
bool CCANPort::AddPacket(bool in_bBroadcast, u8 in_u8Address, void* in_pBuffer, size_t in_stSize)
{
   bool l_bResult = false;

   size_t l_stSize = in_stSize;
   u8* l_pPtr = (u8*)in_pBuffer;

   // Запрещение прерывания получения CAN кадров
   if(m_pAccess)
      m_pAccess(this, 0);

   // Вычисление количества CAN кадров
   u8 l_u8Frames = (l_stSize + 7) / 8;

   // Проверим поместятся ли данные в буфер
   if((m_OutBuffer.m_stMax - m_OutBuffer.m_stCount) >= l_u8Frames)
   {
      // Вычисление указателя на первый CAN кадр
      size_t l_stIndex = (m_OutBuffer.m_stStart + m_OutBuffer.m_stCount) % m_OutBuffer.m_stMax;
      u8 l_u8TID = GenerateTID();
      // Разбивка на буфера на CAN кадры
      for(u8 i = 0; i < l_u8Frames; i++)
      {
         can_frame_t* l_pFrame = m_OutBuffer.m_pBuffer + l_stIndex;

         // Заполнение полей структуры
         l_pFrame->m_u32ExtID =  ((m_u16CANID & IRIDIUM_EXT_ID_CAN_ID_MASK) << IRIDIUM_EXT_ID_CAN_ID_SHIFT) |
                                 ((l_u8TID & IRIDIUM_EXT_ID_TID_MASK) << IRIDIUM_EXT_ID_TID_SHIFT) |
                                 (in_bBroadcast << IRIDIUM_EXT_ID_BROADCAST_SHIFT) |
                                 (in_u8Address << IRIDIUM_EXT_ID_ADDRESS_SHIFT) |
                                 (l_u8Frames == (i + 1));

         l_pFrame->m_u8Size = (l_stSize < 8) ? l_stSize : 8;
         // Копирование данных
         memcpy(l_pFrame->m_aData, l_pPtr, l_pFrame->m_u8Size);
         // Уменьшение размера
         l_stSize -= l_pFrame->m_u8Size;
         l_pPtr += l_pFrame->m_u8Size;

         l_stIndex = (l_stIndex + 1) % m_OutBuffer.m_stMax;
      }
      // Увеличение количества CAN кадров
      m_OutBuffer.m_stCount += l_u8Frames;
      l_bResult = true;
   } else
   {
      l_bResult = false;
   }

   // Разрешение прерывания получения CAN кадра
   if(m_pAccess)
      m_pAccess(this, 1);

   return l_bResult;
}

/**
   Получение текущего пакета
   на входе    :  out_rID     - ссылка на переменную куда нужно поместить идентифкатор пакета
                  out_rBuffer - ссылка на указатель куда нужно поместить указатель на данные полученого пакета
                  out_rSize   - ссылка на переменную куда нужно поместить размер полученого пакета
   на выходе   :  успешность получения пакета
*/
bool CCANPort::GetPacket(u32& out_rID, void*& out_rBuffer, size_t& out_rSize)
{
   u32 l_u32ID = 0;
   // Запрещение прерывания получения CAN кадров
   if(m_pAccess)
      m_pAccess(this, 0);

   // Проверка наличия в буфере ранее полученого пакета
   bool l_bResult = (0 != m_stPacketSize);
   if(!l_bResult)
   {
      // Поиск замыкающего CAN кадра
      can_frame_t* l_pEnd = m_InBuffer.m_pBuffer + m_InBuffer.m_stCount;
      for(can_frame_t* l_pCur = m_InBuffer.m_pBuffer; l_pCur != l_pEnd; l_pCur++)
      {
         // Поиск замыкающего CAN кадра
         if(l_pCur->m_u32ExtID & IRIDIUM_EXT_ID_END_MASK)
         {
            // Запомним идентифкатор пакета
            l_u32ID = l_pCur->m_u32ExtID;
            // Найден замыкающий CAN кадра, соберем пакет
            m_stPacketSize = Assembly(l_pCur, m_aPacketBuffer, sizeof(m_aPacketBuffer));
            // Результат работы
            l_bResult = (0 != m_stPacketSize);
            break;
         }
      }
   }

   // Если данные получены, вернем указатель и длинну пакета
   if(l_bResult)
   {
      // Вернем уже полученый пакет
      out_rID = l_u32ID;
      out_rBuffer = m_aPacketBuffer;
      out_rSize = m_stPacketSize;
   } else
   {
      if(m_InBuffer.m_stCount == m_InBuffer.m_stMax)
      {
         //printf("CCANPort::GetPacket ERROR count = 0!!!\r\n");
         m_InBuffer.m_stCount = 0;
      }
   }

   // Разрешение прерывания получения CAN кадра
   if(m_pAccess)
      m_pAccess(this, 1);

   return l_bResult;
}

/**
   Удаление обработаного пакета 
   на входе    :  *
   на выходе   :  *
*/
void CCANPort::DeletePacket()
{
   // Запрещение прерывания получения CAN кадров
   if(m_pAccess)
      m_pAccess(this, 0);

   m_stPacketSize = 0;

   // Разрешение прерывания получения CAN кадра
   if(m_pAccess)
      m_pAccess(this, 1);
}

/**
   Удаление всех неиспользуемых CAN кадров
   на входе    :  *
   на выходе   :  *
*/
void CCANPort::Flush()
{
   // Запрещение прерывания получения CAN кадров
   if(m_pAccess)
      m_pAccess(this, 0);

   if(m_InBuffer.m_stCount)
   {
      // Сборка пакета
      can_frame_t* l_pEnd = m_InBuffer.m_pBuffer + m_InBuffer.m_stCount;
      can_frame_t* l_pCur = m_InBuffer.m_pBuffer;
      while(l_pCur != l_pEnd)
      {
         if(!l_pCur->m_u32ExtID)
         {
            // Уменьшение количества CAN кадров
            m_InBuffer.m_stCount--;
            // Удаление CAN кадра из массива
            u8* l_pDst = (u8*)l_pCur;
            u8* l_pSrc = (u8*)(l_pCur + 1);
            size_t l_stSize = (u8*)l_pEnd - l_pSrc;
            if(l_stSize)
               memmove(l_pDst, l_pSrc, l_stSize);
            // Вычисление нового указателя на конец
            l_pEnd = m_InBuffer.m_pBuffer + m_InBuffer.m_stCount;
         } else
            l_pCur++;
      }
   }
   // Разрешение прерывания получения CAN кадра
   if(m_pAccess)
      m_pAccess(this, 1);
}

/**
   Получение указателя на CAN кадр по индексу
   на входе    :  *
   на выходе   :  указатель на данные текущего CAN кадра, если NULL - нет текущего CAN кадра, то есть нет CAN кадров для отправки
*/
can_frame_t* CCANPort::GetFrame()
{
   can_frame_t* l_pResult = NULL;

   // Проверка наличия CAN кадров для отправки
   if(m_OutBuffer.m_stCount)
      l_pResult = m_OutBuffer.m_pBuffer + m_OutBuffer.m_stStart;

   return l_pResult;
}

/**
   Удаление CAN кадров
   на входе    :  *
   на выходе   :  *
*/
void CCANPort::DeleteFrame()
{
   // Проверка наличия CAN кадров
   if(m_OutBuffer.m_stCount)
   {
      m_OutBuffer.m_stStart = (m_OutBuffer.m_stStart + 1) % m_OutBuffer.m_stMax;
      m_OutBuffer.m_stCount--;
   }
}

/**
   Сборка пакета из CAN кадров
   на входе    :  in_pEnd     - указатель на замыкающий CAN кадр
                  out_pBuffer - указатель на буфер куда нужно поместить данные
                  in_stSize   - размер буфера куда нужно поместить данные
   на выходе   :  длинна собранного пакета
*/
size_t CCANPort::Assembly(can_frame_t* in_pEnd, void* out_pBuffer, size_t in_stSize)
{
   size_t l_stResult = 0;
   u8* l_pOut = (u8*)out_pBuffer;
   u32 l_u32Mask = IRIDIUM_EXT_ID_COMPARE_MASK;
   u32 l_u32ID = in_pEnd->m_u32ExtID & l_u32Mask;

   // Сборка пакета
   for(can_frame_t* l_pCur = m_InBuffer.m_pBuffer; l_pCur <= in_pEnd; l_pCur++)
   {
      // Поиск CAN кадра с указанными параметрами
      if((l_pCur->m_u32ExtID & l_u32Mask) == l_u32ID)
      {
         // Получение размера
         size_t l_stSize = (in_stSize > l_pCur->m_u8Size) ? l_pCur->m_u8Size : in_stSize;
         // Добавление содержимого CAN кадра в буфер
         if(l_stSize)
         {
            memcpy(l_pOut, l_pCur->m_aData, l_stSize);
            // Сдвиг позиции в буфере
            l_pOut += l_stSize;
            l_stResult += l_stSize;
            in_stSize -= l_stSize;
         }
         // Отметим что CAN кадр обработан
         l_pCur->m_u32ExtID = 0;
      }
   }
   return l_stResult;
}

/**
   Получение сгенерированного идентификатора транзакции
   на входе    :  *
   на выходе   :  идентификатор транзакции в диапазоне от 0 до 7
*/
u8 CCANPort::GenerateTID()
{
   m_u8TID = (m_u8TID + 1) & 0x07;
   return m_u8TID;
}
