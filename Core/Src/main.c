/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dbger.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern ApplicationTypeDef Appli_state;

FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */
FIL MyFile;                   /* File object */
extern USBH_HandleTypeDef hUSB_Host; /* USB Host handle */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
static void MSC_Application(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	LOG_INIT();
	LOG_DBG("\n\nSTM32V6 U-Disk demo\n");
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_HOST_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		static uint32_t last_tick = 0;
		uint32_t now = HAL_GetTick();
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
		switch(Appli_state)
    {
      case APPLICATION_START:
				LOG_DBG("APPLICATION_START\n");
        MSC_Application();
        Appli_state = APPLICATION_IDLE;
        break;
        
      case APPLICATION_IDLE:
				//LOG_DBG("APPLICATION_IDLE\n");
				if(now - last_tick > 3000) {
					last_tick = now;
					LOG_DBG("set Appli_state to START\n");
					MSC_Application();
				}
				break;
			
			case APPLICATION_READY:
				LOG_DBG("APPLICATION_READY\n");
				break;
			
			case APPLICATION_DISCONNECT:
				LOG_DBG("APPLICATION_DISCONNECT\n");
				break;
			
      default:
        break;      
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void MSC_Application(void)
{
  FRESULT res;                                          /* FatFs function common result code */
  uint32_t byteswritten, bytesread;                     /* File write/read counts */
  uint8_t wtext[] = "This is STM32 working with FatFs"; /* File write buffer */
  uint8_t rtext[100];                                   /* File read buffer */
  
  /* Register the file system object to the FatFs module */
  if(f_mount(&USBDISKFatFs, (TCHAR const*)USBHPath, 0) != FR_OK)
  {
    LOG_ERR("f_mount err\n");
    Error_Handler();
  }
  else
  {
		LOG_DBG("f_mount success\n");
    /* Create and Open a new text file object with write access */
		res = f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) 
    {
			f_mount(0, (TCHAR const*)USBHPath, 0);
      LOG_ERR("f_open for write err[%d]\n", res);
      Error_Handler();
    }
    else
    {
			LOG_DBG("f_open for write success\n");
      /* Write data to the text file */
      res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);
      
      if((byteswritten == 0) || (res != FR_OK))
      {
				f_mount(0, (TCHAR const*)USBHPath, 0);
        LOG_ERR("f_write err\n");
        Error_Handler();
      }
      else
      {
				LOG_DBG("f_write success\n");
        /* Close the open text file */
        f_close(&MyFile);
        
        /* Open the text file object with read access */
        if(f_open(&MyFile, "STM32.TXT", FA_READ) != FR_OK)
        {
					f_mount(0, (TCHAR const*)USBHPath, 0);
          LOG_ERR("f_open for read err\n");
          Error_Handler();
        }
        else
        {
					LOG_DBG("f_open for read success\n");
          /* Read data from the text file */
          res = f_read(&MyFile, rtext, sizeof(rtext), (void *)&bytesread);
          
          if((bytesread == 0) || (res != FR_OK))
          {
						f_mount(0, (TCHAR const*)USBHPath, 0);
            LOG_ERR("f_read err\n");
            Error_Handler();
          }
          else
          {
						LOG_DBG("f_read success\n");
            /* Close the open text file */
            f_close(&MyFile);
            
            /* Compare read data with the expected data */
            if((bytesread != byteswritten))
            {
							f_mount(0, (TCHAR const*)USBHPath, 0);
              LOG_ERR("f_close err\n");
              Error_Handler();
            }
            else
            {
              /* Success of the demo: no error occurrence */
							LOG_DBG("dome test success\n");
            }
          }
        }
      }
    }
		f_mount(0, (TCHAR const*)USBHPath, 0);
  }
  
  /* Unlink the USB disk I/O driver */
  //FATFS_UnLinkDriver(USBHPath);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
