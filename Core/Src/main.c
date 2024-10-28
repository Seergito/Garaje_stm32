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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "TM1638.h"
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
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Define el número total de pasos que puede moverse la barrera
#define PASOS 8
// Contiene la configuración de pines y puertos para CLK, STB y DIO
TM1638 TM;

// Enumeración que define todos los estados posibles de la barrera
enum estados {
	stCerrado,          // Estado: Barrera completamente cerrada
	stAbierto,          // Estado: Barrera completamente abierta
	stCerrando,         // Estado: Barrera en proceso de cierre
	stAbriendo,         // Estado: Barrera en proceso de apertura
	stParadoAbriendo,   // Estado: Barrera detenida durante la apertura
	stParadoCerrado     // Estado: Barrera detenida durante el cierre
};

// Inicialización del estado inicial como cerrado
enum estados estado = stCerrado;

// Variables para el control del botón B1
int flancoP_B1 = 0; // Detecta el flanco positivo del botón (1 cuando se presiona)
int last_value_B1;      // Almacena el último estado leído del botón
int value_B1;           // Almacena el estado actual del botón

// Control de posición de la barrera
int posicion = 0;    // 0 = completamente cerrada, PASOS = completamente abierta

// Temporizador para controlar intervalos de movimiento
uint32_t timer; // Almacena marcas de tiempo para controlar la velocidad de movimiento
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */


	// Inicialización de los periféricos necesarios
	MX_GPIO_Init();           // Inicializa los pines GPIO
	MX_USART2_UART_Init();    // Inicializa la comunicación UART
	MX_TIM3_Init();           // Inicializa el temporizador 3

	/* USER CODE BEGIN 2 */
	// Configuración del display TM1638
	TM.clk_port = CLK_GPIO_Port;   // Asigna el puerto del reloj
	TM.clk_pin = CLK_Pin;          // Asigna el pin del reloj
	TM.stb_port = STB_GPIO_Port;   // Asigna el puerto del strobe
	TM.stb_pin = STB_Pin;          // Asigna el pin del strobe
	TM.dio_port = DIO_GPIO_Port;   // Asigna el puerto de datos
	TM.dio_pin = DIO_Pin;          // Asigna el pin de datos

	// Inicialización del display
	tm1638_TurnOn(&TM, 7);         // Enciende el display con brillo máximo (7)
	tm1638_DisplayClear(&TM);       // Limpia el contenido del display

	char buffer[9];                  // Buffer para almacenar texto a mostrar

	// Función para mostrar el estado de los LEDs según la posición
	void ShowLed(int leds) {
		for (int i = 1; i <= 8; i++) {
			tm1638_Led(&TM, i, i <= leds); // Enciende los LEDs hasta la posición actual
		}
	}

	// Función que verifica si no hay obstáculos en la barrera
	bool BarreraOk() {
		uint8_t buttons = tm1638_ScanButtons(&TM); // Lee el estado de los botones
		return !tm1638_KeyState(&TM, buttons, 1); // Retorna true si no hay obstáculo
	}

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	// Guarda el último estado del botón B1 para detectar cambios
	last_value_B1 = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

	// Bucle principal del programa
	while (1) {
		// Lee el estado actual del botón B1
		value_B1 = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
		// Detecta flanco positivo (transición de alto a bajo)
		flancoP_B1 = (last_value_B1 != value_B1) && !value_B1;

		// Debounce del botón
		if (flancoP_B1)
			HAL_Delay(500);

		//Máquina de estados - Transiciones
		switch (estado) {
		case stCerrado:
			if (flancoP_B1) {              // Si se presiona el botón
				estado = stAbriendo;        // Cambia al estado de apertura
				timer = HAL_GetTick();      // Inicia el temporizador
			}
			break;

		case stAbierto:
			if (flancoP_B1) {              // Si se presiona el botón
				estado = stCerrando;        // Cambia al estado de cierre
				timer = HAL_GetTick();      // Inicia el temporizador
			}
			break;

		case stCerrando:
			if (flancoP_B1) {              // Si se presiona el botón
				estado = stParadoCerrado;   // Detiene el cierre
			} else {
				// Verifica si ha pasado 1 segundo
				if ((HAL_GetTick() - timer) > 1000) {
					if (BarreraOk())        // Si no hay obstáculos
						posicion--;          // Decrementa la posición
					timer = HAL_GetTick();   // Reinicia el temporizador
				}
			}
			// Si llegó a la posición cerrada
			if (posicion == 0)
				estado = stCerrado;
			break;

		case stAbriendo:
			if (flancoP_B1) {              // Si se presiona el botón
				estado = stParadoAbriendo;  // Detiene la apertura
			} else {
				// Verifica si ha pasado 1 segundo
				if ((HAL_GetTick() - timer) > 1000) {
					posicion++;             // Incrementa la posición
					timer = HAL_GetTick();  // Reinicia el temporizador
				}
			}
			// Si llegó a la posición abierta
			if (posicion == PASOS)
				estado = stAbierto;
			break;

		case stParadoAbriendo:
			if (flancoP_B1) {              // Si se presiona el botón
				estado = stCerrando;        // Comienza el cierre
			}
			break;

		case stParadoCerrado:
			if (flancoP_B1) {              // Si se presiona el botón
				estado = stAbriendo;        // Comienza la apertura
			}
			break;
		}

		//Máquina de estados - Acciones
		switch (estado) {
		case stCerrado:
			tm1638_DisplayTxt(&TM, "cLoSE");  // Muestra "CLOSE" en el display
			break;

		case stAbierto:
			tm1638_DisplayTxt(&TM, "oPEn");   // Muestra "OPEN" en el display
			break;

		case stAbriendo:
			sprintf(buffer, "%02d", PASOS - posicion);  // Formatea el contador
			tm1638_DisplayTxt(&TM, buffer);    // Muestra la cuenta regresiva
			break;

		case stCerrando:
			if (!BarreraOk())
				tm1638_DisplayTxt(&TM, "bArrErA");  // Indica obstáculo
			else {
				sprintf(buffer, "%02d", posicion);   // Formatea la posición
				tm1638_DisplayTxt(&TM, buffer);    // Muestra la posición actual
			}
			break;

		case stParadoAbriendo:
		case stParadoCerrado:
			tm1638_DisplayTxt(&TM, "PArAdo");  // Indica estado detenido
			break;
		}

		// Actualiza los LEDs según la posición actual
		ShowLed(PASOS - posicion);
	}
	/* USER CODE END WHILE */
	/* USER CODE BEGIN 3 */
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void) {

	/* USER CODE BEGIN TIM3_Init 0 */

	/* USER CODE END TIM3_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 65535;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LD2_Pin | STB_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, CLK_Pin | DIO_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LD2_Pin STB_Pin */
	GPIO_InitStruct.Pin = LD2_Pin | STB_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : PA8 */
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : CLK_Pin DIO_Pin */
	GPIO_InitStruct.Pin = CLK_Pin | DIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
