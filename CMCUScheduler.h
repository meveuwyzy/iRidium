#ifndef _C_MCU_SCHEDULER_H_INCLUDED_
#define _C_MCU_SCHEDULER_H_INCLUDED_

#include "IridiumValue.h"

#define SCHEDULE_BYTES_SIZE ((7*24*60)/8)          // Размер битового поля в байтах
#define MAX_SCHEDULES      16                      // Максимальное количество расписаний

// Событие
enum eScheduleEvent
{
   SCHEDULE_EVENT_OFF = 0,                         // Переход с 1 до 0 значения
   SCHEDULE_EVENT_ON                               // Переход с 0 до 1 значения
};

// Зона действия переменной
enum eScheduleVariableArea
{
   SCHEDULE_VARIABLE_AREA_NONE = 0,                // Нет
   SCHEDULE_VARIABLE_AREA_LOCAL,                   // Локальная переменная
   SCHEDULE_VARIABLE_AREA_GLOBAL                   // Глобальная переменная
};

// Состояние расписания
enum eScheduleState
{
   SCHEDULE_STATE_NONE = 0,                        // Неопределенное остояние
   SCHEDULE_STATE_OFF,                             // Выключено
   SCHEDULE_STATE_ON                               // Включено
};

// Структура для хранения 
typedef struct schedule_state_s
{
   //unsigned m_bEnable   :  1;                      // Флаг доступности расписания
   unsigned m_u2Value   :  2;                      // Текущее значение состояния расписания (eScheduleState)

} schedule_state_t;

// Тип обработчика обратного вызова для указания изменения значения
typedef void (*schedule_callback_t)(u8 in_u8Schedule, size_t in_stUserData, u16 in_u16VariableID, eIridiumValueType in_eType, universal_value_t& in_rValue);

/**
   Класс для работы с расписанием на микроконтроллере
*/
class CMCUScheduler
{
public:
   // Конструктор/деструктор
   CMCUScheduler();
   virtual ~CMCUScheduler();

   // Обработка расписания
   void Work(size_t in_stIndex);

   // Установка/получение максимального размера расписания
   void SetMaxSize(size_t in_stSize)
      { m_stMaxSize = in_stSize; }
   size_t GetMaxSize()
      { return m_stMaxSize; }
   // Взвод флага работы с расписанием
   void Start()
      { m_bWork = true; }
   // Взвод флага прекращения работы с расписанием
   void Stop()
      { m_bWork = false; }
   // Получение количества расписаний
   u8 GetCount()
      { return m_u8Count; }

   // Получение индекса по временной метке
   size_t TimeToIndex(iridium_time_t& in_rTime);
   void IndexToTime(size_t in_stIndex, iridium_time_t& out_rTime);

   // Установка/получение значения
   void Set(u8 in_u8Schedule, u8 in_u8DoW, u8 in_u8Hour, u8 in_u8Minutes, bool in_bValue);
   bool Get(u8 in_u8Schedule, u8 in_u8DoW, u8 in_u8Hour, u8 in_u8Minutes);

   // Получениа количества записанных расписаний
   u8 GetActive();
   
   // Получение/установка сериализованных данных
   size_t GetData(u8* out_pBuffer, size_t in_stSize);
   bool SetData(u8* in_pBuffer, size_t in_stSize);

   // Установка слушателя
   void SetCallback(schedule_callback_t in_pCallback, size_t in_stUserData);

   // Сброс состояния расписания
   void ResetState();

   // Установка указателя на доступность расписаний
   void SetEnable(u8* in_pPtr)
      { m_pEnable = in_pPtr; }
   // Включение/выключение расписания
   void Enable(u8 in_u8Schedule, bool in_bEnable);
   void EnableAll(bool in_bEnable);

   // Установка/получение зоны действия переменной
   eScheduleVariableArea GetVariableArea(u8 in_u8Schedule, eScheduleEvent in_eType);
   void SetVariableArea(u8 in_u8Schedule, eScheduleEvent in_eType, eScheduleVariableArea in_eVariableArea);

   // Установка/получение типа переменной события
   eIridiumValueType GetVariableType(u8 in_u8Schedule, eScheduleEvent in_eType);
   void SetVariableType(u8 in_u8Schedule, eScheduleEvent in_eType, eIridiumValueType in_eVariableArea);

   // Установка/получение значения события
   u64 GetValue(u8 in_u8Schedule, eScheduleEvent in_eType);
   void SetValue(u8 in_u8Schedule, eScheduleEvent in_eType, u64 in_u64Value);

   // Установка/получение глобальной переменной связанной с событием
   u16 GetVariable(u8 in_u8Schedule, eScheduleEvent in_eType);
   void SetVariable(u8 in_u8Schedule, eScheduleEvent in_eType, u16 in_u16Variable);


private:
   // Установка/получение состояния бита
   void SetBit(u8* in_pBuffer, size_t in_stIndex, bool in_bValue);
   bool GetBit(u8* in_pBuffer, size_t in_stIndex);

   // Получение дня недели по дате
   u8 GetDayOfWeek(u16 in_u16Year, u8 in_u8Month, u8 in_u8Day);

   schedule_callback_t  m_pCallback;               // Указатель на обработчик обратного вызова
   size_t               m_stUserData;              // Данные пользователя которые будут передаваться в обработчик обратного вызова
   size_t               m_stOldIndex;              // Обработанный индекс
   schedule_state_t     m_aState[MAX_SCHEDULES];   // Текущее состояние расписаний
   u8*                  m_pEnable;                 // Указатель на доступность расписаний
   size_t               m_stMaxSize;               // Максимальный размер расписания
   bool                 m_bWork;                   // Флаг работы обработчика расписаний
   // Неизменяемые параметры
   u8                   m_u8Count;                 // Количество элементов в расписании
   u8*                  m_pBits;                   // Указатель на массив битовых значений расписания
   // Значение при переходе от 0 к 1
   u8*                  m_pOnTypes;                // Указатель на список типов [R][R][T][T][T][T][T][T]
   u64*                 m_pOn;                     // Указатель на массив значений при переходе значения из 0 в 1 значение
   u16*                 m_pOnVariable;             // Указатель на массив глобальных переменных при переходе из 0 в 1 значение
   // Значение при переходе от 1 к 0
   u8*                  m_pOffTypes;               // Указатель на список типов [R][R][T][T][T][T][T][T]
   u64*                 m_pOff;                    // Указатель на массив значений при переходе значения из 1 в 0 значение
   u16*                 m_pOffVariable;            // Указатель на массив глобальных переменных при переходе из 1 в 0 значение
};
#endif   // _C_MCU_SCHEDULER_H_INCLUDED_
