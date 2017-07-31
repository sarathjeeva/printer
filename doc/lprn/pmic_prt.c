#include <linux/power_supply.h>
#include <linux/pmic_adc.h>
#include <linux/pmic_status.h>
#include <mach/hardware.h>

/* battery adc value to voltage multiplier*/
#define BAT_VOLTAGE_UNIT_UV 				4806

#define PRT_TEMP_GPO1_VOLTAGE 2780000
#define PRT_TEMP_VOLTAGE_UNIT_MV  2390

#define PRT_GPO1_RES	10000   /* 10K resistance */

/* the res to temp table -20 to 79 */
unsigned int Prt_res_table[] = {
  717000, 535000, 405000, 308000, 238000, 185000, 145000, 113000,   /* -40  --> -5 */
  88700, 69900, 55400, 44100, 35400, 28500, 18300, 14900, 12100, 9920, /* 0 --> 50 */
  8160, 6760, 5620, 4700, 3950, 3340, /* 55 --> 80 */
};

unsigned int Prt_res_changer(unsigned int res)
{
   unsigned int i;
   unsigned int temp = 0;

   for (i = 0; i < 24; i++)
   {
      if (res >= Prt_res_table[i])
      {
         temp = i;

         if (temp <= 8)         /* -40~ 0 */
         {
            temp = 8 - temp;
            temp = temp * 5;
            //temp = temp | 0x80000000;
         }
         else
         {
            temp = temp - 7;   /* 5 - 80 */
            temp = temp * 5;
         }

         return temp;
      }
   }

   temp = 0;

   return temp;

}


static unsigned int Prt_vol_to_Res(unsigned int battery_vol)
{

      unsigned int res_voltage = 2780000 - battery_vol;
      unsigned int battery_res;

      battery_res = PRT_TEMP_GPO1_VOLTAGE * (PRT_GPO1_RES / 10) / res_voltage;
      return (battery_res * 10 - PRT_GPO1_RES);
}


static int pmic_get_prt_temp(unsigned short *temp)
{
   t_channel channel;
   unsigned short result[8];

   channel = GEN_PURPOSE_AD6;
   CHECK_ERROR(pmic_adc_convert(channel, result));
   *temp = result[0];

   return 0;
}

/* battery voltage value */
static int pmic_get_batt_voltage(unsigned short *voltage)
{
   t_channel channel;
   unsigned short result[8];

   channel = BATTERY_VOLTAGE;
   CHECK_ERROR(pmic_adc_convert(channel, result));
   *voltage = result[0];

   return 0;
}


int pmic_prt_temp(void)
{
	unsigned short temp = 0;
	int retval;
	unsigned short prt_temp;

	 retval = pmic_get_prt_temp(&prt_temp);
   
   	if (retval == 0)
   	{
		unsigned int prt_res;
		unsigned int prt_vol =  prt_temp;
		prt_vol =  prt_vol* PRT_TEMP_VOLTAGE_UNIT_MV;
   
      		//printk("%s:%d: prt_temp = 0x%x\n", __func__, __LINE__, prt_temp);
		//printk("%s:%d: prt_vol = %d\n", __func__, __LINE__, prt_vol);	
 	
		 prt_res = Prt_vol_to_Res(prt_vol);
		 //printk("%s:%d: prt_res = %d\n", __func__, __LINE__, prt_res);

       	temp = Prt_res_changer(prt_res); 
	 	//printk("%s:%d: prn_temp = %d\n", __func__, __LINE__, temp);
   }

	return temp;
}
//EXPORT_SYMBOL(pmic_prt_temp);

int  pmic_prt_vol(void)
{
#if 0
   unsigned short voltage_raw;
   int voltage_uV=0;	 
	int retval;

   /* last get battery voltage */
   retval = pmic_get_batt_voltage(&voltage_raw);

   if (retval != 0)
   {
      printk("batt voltage raw read error\n");
	  return 4000;   // 4000mV
   }

   /* current voltage */
   voltage_uV = voltage_raw * BAT_VOLTAGE_UNIT_UV;

   /* get mv */
   voltage_uV = voltage_uV / 1000;

   return voltage_uV;
#else
extern int pmic_current_battery_warning(void);

	return pmic_current_battery_warning();
#endif
}


