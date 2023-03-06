#include "CMCUScheduler.h"
#include "IridiumBytes.h"
#include "IridiumCRC16.h"

/**
   Конструктор класса
   на входе    :  *
*/
CMCUScheduler::CMCUScheduler()
{
   m_pCallback    = NULL;
   m_stUserData   = 0;
   m_stOldIndex   = ~0;
   memset(m_aState, 0, sizeof(m_aState));
   m_pEnable      = NULL;
   m_stMaxSize    = 0;
   m_bWork        = false;

   // Неизменяемые параметры
   m_u8Count      = 0;
   m_pBits        = NULL;
   m_pOnTypes     = NULL;
   m_pOn          = NULL;
   m_pOnVariable  = NULL;
   m_pOffTypes    = NULL;
   m_pOff         = NULL;
   m_pOffVariable = NULL;
}

/**
   Деструктор класса
*/
CMCUScheduler::~CMCUScheduler()
{
}

/**
   Обработка данных
   на входе    :  in_stIndex  - индекс времени
   на выходе   :  *
*/
void CMCUScheduler::Work(size_t in_stIndex)
{
   // Получение указателя на данные
   u8* l_pBits = m_pBits;
   if(l_pBits && m_bWork)
   {
      // Проверка изменения
      if(in_stIndex != m_stOldIndex)
      {
         // Обработка списка 
         for(u8 i = 0; i < m_u8Count; i++)
         {
            // Проверка доступно ли расписание
            u8 l_u8Index = i >> 3;
            u8 l_u8Mask = 1 << (i & 7);
            if(m_pEnable && m_pEnable[l_u8Index] & l_u8Mask)
            {
               // Проверка состояния
               u8 l_u8Value = GetBit(l_pBits, in_stIndex) ? SCHEDULE_STATE_ON : SCHEDULE_STATE_OFF;
               if(l_u8Value != m_aState[i].m_u2Value)
               {
                  u16 l_u16Variable = 0;
                  eIridiumValueType l_eType = IVT_NONE;
                  universal_value_t l_Val;

                  // Вызов обработчика обратного вызова если таковой есть
                  if(m_pCallback)
                  {
                     if(SCHEDULE_STATE_ON == l_u8Value)
                     {
                        // Обработка перехода от 0 в 1
                        l_u16Variable     = m_pOnVariable[i];
                        l_eType           = (eIridiumValueType)(m_pOnTypes[i] & 0x3f);
                        ReadLE((u8*)(m_pOn + i), &l_Val.m_u64Value, sizeof(u64));

                     } else
                     {
                        // Обработка перехода от 1 в 0
                        l_u16Variable     = m_pOffVariable[i];
                        l_eType           = (eIridiumValueType)(m_pOffTypes[i] & 0x3f);
                        ReadLE((u8*)(m_pOff + i), &l_Val.m_u64Value, sizeof(u64));
                     }
                     // Вызов обработчика
                     if(l_u16Variable)
                        m_pCallback(i, m_stUserData, l_u16Variable, l_eType, l_Val);
                  }

                  // Обновление состояния
                  m_aState[i].m_u2Value = l_u8Value;
               }
               // Переход на следующее расписание
               l_pBits += SCHEDULE_BYTES_SIZE;
            }
         }
         // Сохранение обработанного индекса
         m_stOldIndex = in_stIndex;
      }
   }
}

/**
   Получение индекса в буфере по метке времени
   на входе    :  in_rTime - ссылка на время
   на выходе   :  индекс в буфере
*/
size_t CMCUScheduler::TimeToIndex(iridium_time_t& in_rTime)
{
   return ((GetDayOfWeek(in_rTime.m_u16Year, in_rTime.m_u8Month, in_rTime.m_u8Day) + 6) % 7) * 24 * 60 + in_rTime.m_u8Hour * 60 + in_rTime.m_u8Minute;
}

/**
   Получение индекса в буфере по метке времени
   на входе    :  in_stIndex  - индекс в буфере
                  out_rTime   - ссылка на время, куда нужно поместить данные
   на выходе   :  *
*/
void CMCUScheduler::IndexToTime(size_t in_stIndex, iridium_time_t& out_rTime)
{
   size_t l_stRemain = in_stIndex;

   out_rTime.m_u8DayOfWeek = l_stRemain / (24 * 60);
   l_stRemain %= (24 * 60);

   out_rTime.m_u8Hour = l_stRemain / 60;
   out_rTime.m_u8Minute = l_stRemain % 60;
}

/**
   Установка значения расписания
   на входе    :  in_u8Schedule  - номер расписания
                  in_u8DoW       - день недели
                  in_u8Hour      - час
                  in_u8Minutes   - минута
                  in_bValue      - значение
   на выходе   :  *
*/
void CMCUScheduler::Set(u8 in_u8Schedule, u8 in_u8DoW, u8 in_u8Hour, u8 in_u8Minutes, bool in_bValue)
{
   if(in_u8Schedule < m_u8Count)
   {
      size_t l_stIndex = in_u8DoW * 24 * 60 + in_u8Hour * 60 + in_u8Minutes;
      u8* l_pPtr = m_pBits + in_u8Schedule * SCHEDULE_BYTES_SIZE;
      SetBit(l_pPtr, l_stIndex, in_bValue);
   }
}

/**
   Получение значения расписания
   на входе    :  in_u8Schedule  - номер расписания
                  in_u8DoW       - день недели
                  in_u8Hour      - час
                  in_u8Minutes   - минута
   на выходе   :  значение
*/
bool CMCUScheduler::Get(u8 in_u8Schedule, u8 in_u8DoW, u8 in_u8Hour, u8 in_u8Minutes)
{
   bool l_bResult = false;

   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      size_t l_stIndex = in_u8DoW * 24 * 60 + in_u8Hour * 60 + in_u8Minutes;
      u8* l_pPtr = m_pBits + in_u8Schedule * SCHEDULE_BYTES_SIZE;
      l_bResult = GetBit(l_pPtr, l_stIndex);
   }
   return l_bResult;
}

/**
   Получение количества записанных расписаний
   на входе    :  *
   на выходе   :  количество записанных расписаний
*/
u8 CMCUScheduler::GetActive()
{
   u8* l_pBits = m_pBits;
   u8 l_u8result = 0;
   if(l_pBits)
   {
      for(size_t i = 0; i < MAX_SCHEDULES; i++)
      {
         for(size_t j = 0; j < SCHEDULE_BYTES_SIZE; j++)
         {
            if(l_pBits[j])
            {
               l_u8result++;
               break;
            }
         }
         l_pBits += SCHEDULE_BYTES_SIZE;
      }
   }
   return l_u8result;
}

/**
   Получение сериализованных данных расписания
   на входе    :  out_pBuffer - указатель на буфер куда нужно поместить данные
                  in_stSize   - размер буфера куда нужно поместить данные
   на выходе   :  размер данных
*/
size_t CMCUScheduler::GetData(u8* out_pBuffer, size_t in_stSize)
{
   u16 l_u16Result = 0;
   // Проверка входных данных
   if(out_pBuffer && in_stSize)
   {
      // Зарезервируем место под размер данных
      u8* l_pPtr = WriteLE(out_pBuffer, &l_u16Result, sizeof(l_u16Result));
      // Запись количества расписаний
      l_pPtr = WriteLE(l_pPtr, &m_u8Count, sizeof(m_u8Count));
      // Массив битов определяющих расписания
      for(u8 i = 0; i < m_u8Count; i++)
         l_pPtr = WriteLE(l_pPtr, m_pBits + i * SCHEDULE_BYTES_SIZE, SCHEDULE_BYTES_SIZE);

      // Массив типов значения при переходе с 0 на 1
      for(u8 i = 0; i < m_u8Count; i++)
         l_pPtr = WriteLE(l_pPtr, &m_pOnTypes[i], sizeof(m_pOnTypes[0]));
      // Массив значений при переходе с 0 на 1
      for(u8 i = 0; i < m_u8Count; i++)
         l_pPtr = WriteLE(l_pPtr, &m_pOn[i], sizeof(m_pOn[0]));
      // Массив идентификаторов при переходе с 0 на 1
      for(u8 i = 0; i < m_u8Count; i++)
         l_pPtr = WriteLE(l_pPtr, &m_pOnVariable[i], sizeof(m_pOnVariable[0]));

      // Массив типов значения при переходе с 1 на 0
      for(u8 i = 0; i < m_u8Count; i++)
         l_pPtr = WriteLE(l_pPtr, &m_pOffTypes[i], sizeof(m_pOffTypes[0]));
      // Массив значений при переходе с 1 на 0
      for(u8 i = 0; i < m_u8Count; i++)
         l_pPtr = WriteLE(l_pPtr, &m_pOff[i], sizeof(m_pOff[0]));
      // Массив идентификаторов при переходе с 1 на 0
      for(u8 i = 0; i < m_u8Count; i++)
         l_pPtr = WriteLE(l_pPtr, &m_pOffVariable[i], sizeof(m_pOffVariable[0]));

      // Вычисление размера данных (размер данных + размер CRC16)
      l_u16Result = (l_pPtr - out_pBuffer) + 2;
      // Запись размера данных
      WriteLE(out_pBuffer, &l_u16Result, sizeof(l_u16Result));
      // Вычисление CRC16
      u16 l_u16CRC = GetCRC16Modbus(0x7700, out_pBuffer, l_u16Result - 2);
      // Запись CRC16 в файл
      l_pPtr = WriteLE(l_pPtr, &l_u16CRC, sizeof(l_u16CRC));
   }
   return l_u16Result;
}

/**
   Установка сериализованных данных 
   на входе    :  in_bClear   - флаг принудительного удаления старого расписания
                  in_pBuffer  - указатель на сериализованные данные расписания                  
                  in_stSize   - размер данных
   на выходе   :  результат работу
*/
bool CMCUScheduler::SetData(u8* in_pBuffer, size_t in_stSize)
{
   bool l_bResult = false;
   u16 l_u16Size = 0;

   // Предварительное удаление расписания
   m_u8Count      = 0;
   m_pBits        = NULL;
   m_pOnTypes     = NULL;
   m_pOn          = NULL;
   m_pOnVariable  = NULL;
   m_pOffTypes    = NULL;
   m_pOff         = NULL;
   m_pOffVariable = NULL;

   // Проверка входящих данных
   if(in_pBuffer && in_stSize)
   {
      // Чтение размера данных
      u8* l_pPtr = ReadLE(in_pBuffer, &l_u16Size, sizeof(l_u16Size));

      // Проверка размера
      if(l_u16Size <= m_stMaxSize)
      {
         // Проверка контрольной суммы
         if(0 == GetCRC16Modbus(0x7700, in_pBuffer, l_u16Size))
         {
            // Чтение количества расписаний
            l_pPtr = ReadLE(l_pPtr, &m_u8Count, sizeof(m_u8Count));
            // Массив с битами
            m_pBits = l_pPtr;
            l_pPtr += m_u8Count * SCHEDULE_BYTES_SIZE;

            // Массив типов
            m_pOnTypes = (u8*)l_pPtr;
            l_pPtr += m_u8Count * sizeof(m_pOnTypes[0]);
            // Массив с значениями
            m_pOn = (u64*)l_pPtr;
            l_pPtr += m_u8Count * sizeof(m_pOn[0]);
            // Массив с идентификаторами
            m_pOnVariable = (u16*)l_pPtr;
            l_pPtr += m_u8Count * sizeof(m_pOnVariable[0]);

            // Массив типов
            m_pOffTypes = (u8*)l_pPtr;
            l_pPtr += m_u8Count * sizeof(m_pOffTypes[0]);
            // Массив с значениями
            m_pOff = (u64*)l_pPtr;
            l_pPtr += m_u8Count * sizeof(m_pOff[0]);
            // Массив с идентификаторами
            m_pOffVariable = (u16*)l_pPtr;
            l_pPtr += m_u8Count * sizeof(m_pOffVariable[0]);
            // Сброс
            ResetState();
            l_bResult = true;
         }
      }
   }

   return l_bResult;
}

/**
   Установка сериализованных данных 
   на входе    :  in_pCallback   - указатель на обработчик обратного вызова
                  in_stUserData  - данные пользователя, которые будут переданы в обработчик обратного вызова
   на выходе   :  *
*/
void CMCUScheduler::SetCallback(schedule_callback_t in_pCallback, size_t in_stUserData)
{
   m_pCallback = in_pCallback;
   m_stUserData = in_stUserData;
}

/**
   Сброс расписания
   на входе    :  *
   на выходе   :  *
*/
void CMCUScheduler::ResetState()
{
   m_stOldIndex = ~0;

   // Сброс значений состояния расписаний
   for(size_t i = 0; i < sizeof(m_aState) / sizeof(m_aState[0]); i++)
      m_aState[i].m_u2Value = SCHEDULE_STATE_NONE;
}

/**
   Включение/выключение доступности указанного расписания
   на входе    :  in_u8Schedule  - номер расписания
                  in_bEnable     - признак доступности расписания
   на выходе   :  *
*/
void CMCUScheduler::Enable(u8 in_u8Schedule, bool in_bEnable)
{
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count && m_pEnable)
   {
      // Получение индекса и маски
      u8 l_u8Index = in_u8Schedule >> 3;
      u8 l_u8Mask = 1 << (in_u8Schedule & 7);

      // Изменение бита
      if(in_bEnable)
         m_pEnable[l_u8Index] |= l_u8Mask;
      else
         m_pEnable[l_u8Index] &= ~l_u8Mask;
   }
}

/**
   Получение зоны переменной
   на входе    :  in_u8Schedule  - номер расписания
                  in_eType       - тип расписания
   на выходе   :  зона перемеенной
*/
eScheduleVariableArea CMCUScheduler::GetVariableArea(u8 in_u8Schedule, eScheduleEvent in_eType)
{
   eScheduleVariableArea l_eResult = SCHEDULE_VARIABLE_AREA_NONE;
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u8* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOffTypes : m_pOnTypes;
      l_eResult = (eScheduleVariableArea)(l_pPtr[in_u8Schedule] >> 6);
   }
   return l_eResult;
}

/**
   Установка зоны переменной
   на входе    :  in_u8Schedule     - номер расписания
                  in_eType          - тип расписания
                  in_eVariableArea  - зона переменной
   на выходе   :  *
*/
void CMCUScheduler::SetVariableArea(u8 in_u8Schedule, eScheduleEvent in_eType, eScheduleVariableArea in_eVariableArea)
{
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u8* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOffTypes : m_pOnTypes;
      u8 l_u8Val = l_pPtr[in_u8Schedule];
      l_pPtr[in_u8Schedule] = ((l_u8Val & 0x3f) | (in_eVariableArea << 6));
   }
}

/**
   Получение типа переменной события
   на входе    :  in_u8Schedule  - номер расписания
                  in_eType       - тип события
   на выходе   :  тип события
*/
eIridiumValueType CMCUScheduler::GetVariableType(u8 in_u8Schedule, eScheduleEvent in_eType)
{
   eIridiumValueType l_eResult = IVT_NONE;
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u8* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOffTypes : m_pOnTypes;
      l_eResult = (eIridiumValueType)(l_pPtr[in_u8Schedule] & 0x3f);
   }
   return l_eResult;
}

/**
   Установка типа переменной события
   на входе    :  in_u8Schedule     - номер расписания
                  in_eType          - тип события
                  in_eVariableType  - тип переменной
   на выходе   :  *
*/
void CMCUScheduler::SetVariableType(u8 in_u8Schedule, eScheduleEvent in_eType, eIridiumValueType in_eVariableType)
{
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u8* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOffTypes : m_pOnTypes;
      u8 l_u8Val = l_pPtr[in_u8Schedule];
      l_pPtr[in_u8Schedule] = ((l_u8Val & ~0x3f) | in_eVariableType);
   }
}

/**
   Получение значения переменной события
   на входе    :  in_u8Schedule  - номер расписания
                  in_eType       - тип события
   на выходе   :  значение переменной
*/
u64 CMCUScheduler::GetValue(u8 in_u8Schedule, eScheduleEvent in_eType)
{
   u64 l_u64Result = 0;
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u64* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOff : m_pOn;
      l_u64Result = l_pPtr[in_u8Schedule];
   }
   return l_u64Result;
}

/**
   Установка значения переменной события
   на входе    :  in_u8Schedule  - номер расписания
                  in_eType       - тип события
                  in_u64Value    - значение переменной
   на выходе   :  *
*/
void CMCUScheduler::SetValue(u8 in_u8Schedule, eScheduleEvent in_eType, u64 in_u64Value)
{
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u64* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOff : m_pOn;
      l_pPtr[in_u8Schedule] = in_u64Value;
   }
}

/**
   Получение идентификатора переменной события
   на входе    :  in_u8Schedule  - номер расписания
                  in_eType       - тип события
   на выходе   :  идентиификатор переменной
*/
u16 CMCUScheduler::GetVariable(u8 in_u8Schedule, eScheduleEvent in_eType)
{
   u16 l_u16Result = 0;
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u16* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOffVariable : m_pOnVariable;
      l_u16Result = l_pPtr[in_u8Schedule];
   }
   return l_u16Result;
}

/**
   Установка идентификатора переменной события
   на входе    :  in_u8Schedule  - номер расписания
                  in_eType       - тип события
                  in_u16Variable - идентиификатор переменной
   на выходе   :  *
*/
void CMCUScheduler::SetVariable(u8 in_u8Schedule, eScheduleEvent in_eType, u16 in_u16Variable)
{
   // Проверка на выход за пределы
   if(in_u8Schedule < m_u8Count)
   {
      u16* l_pPtr = (in_eType == SCHEDULE_EVENT_OFF) ? m_pOffVariable : m_pOnVariable;
      l_pPtr[in_u8Schedule] = in_u16Variable;
   }
}

/**
   Установка значения
   на входе    :  in_pBuffer  - указатель на буфер с значением
                  in_stIndex  - индекс значения
                  in_bValue   - значение
   на выходе   :  *
*/
void CMCUScheduler::SetBit(u8* in_pBuffer, size_t in_stIndex, bool in_bValue)
{
   // Вычисление индекса и номера бита
   size_t l_stIndex = in_stIndex >> 3;
   u8 l_u8Mask = 1 << (in_stIndex & 7);

   // Проверка значения
   if(in_bValue)
      in_pBuffer[l_stIndex] |= l_u8Mask;
   else
      in_pBuffer[l_stIndex] &= ~l_u8Mask;
}

/**
   Получение значения
   на входе    :  in_pBuffer  - указатель на буфер с значением
                  in_stIndex  - индекс значения
   на выходе   :  значение бита
*/
bool CMCUScheduler::GetBit(u8* in_pBuffer, size_t in_stIndex)
{
   // Вычисление индекса и номера бита
   size_t l_stIndex = in_stIndex >> 3;
   u8 l_u8Mask = 1 << (in_stIndex & 7);

   // Вернем значение бита
   return (0 != (in_pBuffer[l_stIndex] & l_u8Mask));
}

/**
   Получение дня недели по дате
   на входе    :  in_u16Yaer  - год
                  in_u8Month  - месяц
                  in_u8Day    - день
   на выходе   :  день недели
                  0 - воскресенье
                  1 - понедельник
                  2 - вторник
                  3 - среда
                  4 - четверг
                  5 - пятница
                  6 - суббота
*/
u8 CMCUScheduler::GetDayOfWeek(u16 in_u16Year, u8 in_u8Month, u8 in_u8Day)
{
   if(in_u8Month < 3)
   {
      --in_u16Year;
      in_u8Month += 10;
   } else
      in_u8Month -= 2;

   return ((in_u8Day + 31 * in_u8Month / 12 + in_u16Year + in_u16Year / 4 - in_u16Year / 100 + in_u16Year / 400) % 7);
}
