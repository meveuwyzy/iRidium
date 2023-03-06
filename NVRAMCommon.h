#ifndef _NVRAM_COMMON_H_INCLUDED_
#define _NVRAM_COMMON_H_INCLUDED_

#include "IridiumTypes.h"

// Режим работы устройства
typedef enum bootloader_mode
{
   BOOTLOADER_MODE_RUN = 0,                        // Запуск прошивки
   BOOTLOADER_MODE_DOWNLOAD,                       // Загрузка прошивки
   BOOTLOADER_MODE_BOOT                            // Переход в режим прошивки
} eBootloaderMode;

// Общая структура хранения данных в энергонезависимой памяти
typedef struct nvram_common_s
{
   u8    m_u8Mode;                                 // Режим работы
   u32   m_u32FirmwareSize;                        // Размер прошивки
   u16   m_u16FirmwareCRC16;                       // Контрольная сумма прошивки
   u16   m_u16FirmwareID;                          // Идентификатор профиля прошивки
   u16   m_u16FirmwareAddress;                     // Адрес устройства инициатора прошивки
   u16   m_u16FirmwareTID;                         // Идентификатр транзакции инициатора прошивки
   u8    m_u8FirmwareRoute;                        // Маршрут инициатора прошивки
   u8    m_u8LID;                                  // Локальный идентификатор устройства
   u8    m_aKey[32];                               // Ключ шифрования 256 бит
   u32   m_u32PIN;                                 // Персональное идентификационное число
   u32   m_u32UserID;                              // Пользовательский идентификатор
   char  m_szDeviceName[64];                       // Имя устройства
   u32   m_u32BlinkTime;                           // Время мерцания светодиода
   u8    m_aVersion[2];                            // Версия прошивки
   u32   m_u32Build;                               // Дата сборки
   u8    m_aReserved[22];                          // Резервируем память
} nvram_common_t;

// Флаги передаваемые через RTC регистры
#define REGISTER_PRESS_FLAG                        0x8000   // Признак перезагрузки устройства с зажатой набортной кнопкой
#define REGISTER_UPDATE_BOOT_FLAG                  0x4000   // Признак обновления загрузчика

// Маска для отделения флагов
#define REGISTER_MASK                              ~(REGISTER_PRESS_FLAG | REGISTER_UPDATE_BOOT_FLAG)
   
#endif   // _NVRAM_COMMON_H_INCLUDED_
