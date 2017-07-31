#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <mach/gpio.h>
#include <linux/spi/spi.h> 
#include <linux/font.h>

#include <linux/delay.h>
#include <linux/time.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/ioctl.h>
//#include <asm/string.h>
// #include <asm/arch/hardware.h>
#include "prn_drv.h"
#include "prn_draw.h"
#include "prn_io.h"


#if  0  //def DEBUG_PRN
static int sld=0;
#endif

#define SPI_BUF_LEN 64
#define PRN_DEVNAME "lprn"
/*------------------global--------------------------*/
struct spi_device *spi_printer;
u8 *g_prn_spi_rdata = NULL;
u8 *g_prn_spi_wdata = NULL;

static Printer prn={
	.open=0,
	.block_div = 3,
	.wr_tag=GRAPHY,
	.status=STATUS_IDLE,
	.transfered=XFER_IDLE,
	.wq = __WAIT_QUEUE_HEAD_INITIALIZER(prn.wq),
	.mutex = __MUTEX_INITIALIZER(prn.mutex),	
};

PPrinter pprn=&prn;

static int paxprn_major = 0;  
//static int paxprn_minor = 0;  
static struct class *paxprn_class;
/*---------------program-------------------------------*/
int DelayMs(int Ms)
{
	udelay(Ms * 1000);
	return 0;
}

#define DelayUs udelay


int spi_printer_setup(struct spi_device *spi, int bit)
{
#if 0
	spi->mode = SPI_MODE_1;//SPI_MODE_3;
	spi->bits_per_word = bit;
#else	
	spi->mode = SPI_MODE_0;//SPI_MODE_3;
	spi->bits_per_word = 8;
#endif

	return spi_setup(spi);
}
/*
static inline int spi_rw(struct spi_device *spi, u8 * buf, size_t len)
{
	struct spi_transfer t = {
		.tx_buf = (const void *)buf,
		.rx_buf = buf,
		.len = len,
		.cs_change = 0,
		.delay_usecs = 0,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	if (spi_sync(spi, &m) != 0 || m.status != 0)
		return -1;
	return len - m.actual_length;
}
*/

#if 0	
static int spi_read_write(struct spi_device *spi, u8 * rx_buf, u8 * tx_buf, u32 len)
{
   struct spi_message m;
   struct spi_transfer t;

//   spi_printer_setup(spi, len);

   spi_message_init(&m);
   memset(&t, 0, sizeof t);

   t.tx_buf = tx_buf;
   t.rx_buf = NULL;//rx_buf;
   t.len = len >> 3;

   spi_message_add_tail(&t, &m);

   if (spi_sync(spi, &m) != 0 || m.status != 0)
   {
      printk(KERN_ERR "%s: error\n", __func__);
      return -1;
   }
	
   return 0;

}
#else
static void spi_prn_complete(void *arg)
{
	pprn->transfered=XFER_FINISHED;

	if(pprn->m.status!=0)
		pprn->transfered=XFER_ERR;
}

extern int spi_bitbang_poll_transfer(struct spi_device *spi, struct spi_message *m);

static int spi_read_write(struct spi_device *spi, u8 * rx_buf, u8 * tx_buf, u32 len)
{
//   spi_printer_setup(spi, len);
   spi_message_init(&pprn->m);
   memset(&pprn->t, 0, sizeof(pprn->t));

   pprn->t.tx_buf = tx_buf;
   pprn->t.rx_buf = NULL;//rx_buf;
   pprn->t.len = len >> 3;

   spi_message_add_tail(&pprn->t, &pprn->m);	
	pprn->m.complete = spi_prn_complete;
	pprn->m.spi = spi;

	pprn->transfered=XFER_BUSY;
  // if (spi_async(spi, &pprn->m) != 0 );
   return spi_bitbang_poll_transfer(spi, &pprn->m);
}
#endif
//int spicount = 0;
/*
static int spi_write_data(struct spi_device *spi, u8 data)
{
	int ret = 0;
	u8 *rdata = g_prn_spi_rdata;
	u8 *wdata = g_prn_spi_wdata;
	*wdata = data;
	ret = spi_read_write(spi, rdata, wdata, 8);
	return ret;
}

extern int mxc_spi_poll_transfer(struct spi_device *spi, struct spi_transfer *t);

static inline int spi_rw_blk(struct spi_device *spi, u8 * buf, size_t len)
{
	struct spi_transfer t = {
		.tx_buf = (const void *)buf,
		.rx_buf = NULL,
		.len = len,
		.cs_change = 0,
		.delay_usecs = 0,
	};

	return mxc_spi_poll_transfer(spi, &t);

}
*/
int spi_write_blk(struct spi_device *spi, u8 *data, int len)
{

//	int n =(len<<3);
	u8 *rdata = g_prn_spi_rdata;
	u8 *wdata = g_prn_spi_wdata;

	if(len>SPI_BUF_LEN)
		len=SPI_BUF_LEN;

	memcpy(wdata, data, len);

	return spi_read_write(spi, rdata, wdata, (len*8));
//	return spi_rw_blk(spi,wdata,len);
}
/*
int spi_config_printer(struct spi_device *spi)
{
	return 0;
}
*/

void s_LatchData(unsigned char *buf)
{
  /*  volatile*/ //int i ;
#if 0 //def DEBUG_PRN	
	printk("Lat:%d\n", sld++);
#endif

#ifndef LATCH_DATA_ONLY
//    enable_LAT();
//	s_PrnLatch(HIGH);
#endif
 //  for(i=0; i<48; i++)
    {
        //spi_snd_ch(buf[i]); 
        //wait_spi_snd_over();
		spi_write_blk(spi_printer, buf, 48);
//		spi_write_data(spi_printer, buf[i]);
    }
  //  DelayUs(1); 
//    disable_LAT();
#ifndef LATCH_DATA_ONLY
//	s_PrnLatch(LOW);   //LOW
//    DelayUs(2);
//	ndelay(300);
// 	s_PrnLatch(HIGH); // HIGH  
 //   enable_LAT();
 #endif
}


void test_spi_data_count(int n)
{
	int j = 0;
	char buf[PRN_LBYTES];
	
	printk("test_spi_data，param = %d\n", n);

	memset(buf,0xaa,PRN_LBYTES);
	for (j = 0; j < n; j++)
    {
//		disable_LAT();
		spi_write_blk(spi_printer, buf, PRN_LBYTES);
		
		DelayMs(1);
//		enable_LAT();
//		DelayMs(1);
//		disable_LAT();
    }	
}

///////////////////////////////////////////////////////
/* define I/O control command */
//以下ioctl cmd 在mu110打印机的驱动文件也有使用 如果修改 需要到w90p710_mu110.c 中做相同的修改
#define PRN_IOC_MAGIC	'P'
#define PRN_IOC_MAXNR		36


#define PRN_IOC_SETSTROBETIMER      _IOW(PRN_IOC_MAGIC, 0, int)
#define PRN_IOC_FEED                _IOW(PRN_IOC_MAGIC, 1, int)
#define PRN_IOC_REWIND              _IOW(PRN_IOC_MAGIC, 2, int)
#define PRN_IOC_STOP                _IOW(PRN_IOC_MAGIC, 3, int)
#define PRN_IOC_GETDOTWIDTH         _IOR(PRN_IOC_MAGIC, 4, int)
#define PRN_IOC_GETSPAREEREA        _IOR(PRN_IOC_MAGIC, 5, int)
#define PRN_IOC_GETUNPRINTLEN       _IOR(PRN_IOC_MAGIC, 6, int)
#define PRN_IOC_GETSTATUS           _IOR(PRN_IOC_MAGIC, 7, int)
#define PRN_IOC_CLRAllBUF           _IOR(PRN_IOC_MAGIC, 8, int)
#define PRN_IOC_SETAREACONSTRAST	_IOR(PRN_IOC_MAGIC, 9, int)
#define PRN_IOC_REALPRINTFLAG	    _IOR(PRN_IOC_MAGIC, 10, int)
#define PRN_IOC_FEEDSTOP            _IOW(PRN_IOC_MAGIC, 11, int)
#define PRN_IOC_SETMOTORSETUPTBL    _IOW(PRN_IOC_MAGIC, 12, int)

/*
PRT_PD_N	CSI2_D12		GPIO4		GPIO[9]		OUTPUT	步进电机驱动工作控制
PRT_FAT_N	CSI2_D13		GPIO4		GPIO[10]	INPUT	步进电机驱动异常输出
PRT_LAT		USBH1_DATA7		GPIO1		GPIO[18]	OUTPUT	打印机SPI片选
PRT_POUT	EIM_CS4 		GPIO2		GPIO[29]	INPUT	打印机缺纸检测输出
A			NANDF_CS1		GPIO3		GPIO[17]	OUTPUT	步进电机驱动
A-			NANDF_CS2		GPIO3		GPIO[18]	OUTPUT	
B			NANDF_CS3		GPIO3		GPIO[19]	OUTPUT	
B-			NANDF_CS4		GPIO3		GPIO[20]	OUTPUT	
PRT_PWR_ON	NANDF_CS6		GPIO3		GPIO[22]	OUTPUT	打印机电源开启
PRT_STB 	NANDF_CS7		GPIO3		GPIO[23]	OUTPUT	打印机加热控制
5V_PWR_ON	EIM_A27			GPIO2		GPIO[21]	OUTPUT	AP主板5V开启
*/

#define PRN_INIT    		_IOW(PRN_IOC_MAGIC, 13, int)
#define PRN_START    		_IOW(PRN_IOC_MAGIC, 14, int)

#define PRN_TEST_PD_ON  	_IOW(PRN_IOC_MAGIC, 15, int)
#define PRN_TEST_PD_OFF  	_IOW(PRN_IOC_MAGIC, 16, int)

#define PRN_TEST_GET_FAT  	_IOR(PRN_IOC_MAGIC, 17, int)

#define PRN_TEST_LAT_ON  	_IOW(PRN_IOC_MAGIC, 18, int)
#define PRN_TEST_LAT_OFF  	_IOW(PRN_IOC_MAGIC, 19, int)

#define PRN_TEST_GET_PAPER  _IOR(PRN_IOC_MAGIC, 20, int)

#define PRN_TEST_A 			_IOW(PRN_IOC_MAGIC, 21, int)
#define PRN_TEST_A_MINUS 	_IOW(PRN_IOC_MAGIC, 22, int)
#define PRN_TEST_B 			_IOW(PRN_IOC_MAGIC, 23, int)
#define PRN_TEST_B_MINUS 	_IOW(PRN_IOC_MAGIC, 24, int)

#define PRN_TEST_POWER_ON 	_IOW(PRN_IOC_MAGIC, 25, int)
#define PRN_TEST_POWER_OFF 	_IOW(PRN_IOC_MAGIC, 26, int)

#define PRN_TEST_STB_ON 	_IOW(PRN_IOC_MAGIC, 27, int)
#define PRN_TEST_STB_OFF 	_IOW(PRN_IOC_MAGIC, 28, int)

#define PRN_TEST_5V_ON  	_IOW(PRN_IOC_MAGIC, 29, int)
#define PRN_TEST_5V_OFF  	_IOW(PRN_IOC_MAGIC, 30, int)

#define PRN_TEST_SPI  		_IOW(PRN_IOC_MAGIC, 31, int)

#define PRN_TEST_MOTOR4  		_IOW(PRN_IOC_MAGIC, 32, int)
#define PRN_TEST_MOTOR8  		_IOW(PRN_IOC_MAGIC, 33, int)

#define PRN_IOC_SETTAG 		_IOW(PRN_IOC_MAGIC, 34, int)
#define PRN_GET_DOTLINE			_IOR(PRN_IOC_MAGIC, 35, int)

#define STROBE_TIME_MAX     800
#define STROBE_TIME_MIN     200
#define FEED_STEP_MAX       8000


/*---------------Attribue------------------*/
static ssize_t prn_show_temperature(struct device *dev,
		struct device_attribute *attr,char *buf)
{
/*
	Attribute:
	devname, pageline,width,bpp,bpm(um)
*/
	int temp;
//	unsigned short status = PrnStatus(-1);	

//	if(status==STATUS_BUSY)
		temp=pprn->temperature;
	 if(temp<0)temp=0;
/*	else{
		temp=s_ReadHeadTemp();

#ifdef DEBUG_PRN
//		printk("temperateure: %d\n", temp);
#endif
	}
*/	
	return sprintf(buf, "%d\n",temp);
}

static ssize_t prn_show_config(struct device *dev,
		struct device_attribute *attr,char *buf)
{
/*
	Attribute:
	devname, pageline,width,bpp,bpm(um)
*/
	return sprintf(buf, "%s,%d,%d,%d,%d\n",PRN_DEVNAME,MAX_DOT_LINE,PRN_LDOTS,1,PRN_DOT_ES);
}
/*
static ssize_t prn_show_font(struct device *dev,
		struct device_attribute *attr,char *buf)
{

	if(pprn->ft!=NULL){
		struct font_desc *ft=pprn->ft;
		return sprintf(buf, "%s,%d,%d\n", ft->name,ft->width,ft->height);
	}

	return sprintf(buf, "%s,%d,%d\n","NOFONT",0,0);
}
*/
static const char *font_list[2]={
	"VGA8x16",
	"VGA8x8",	
};
/*
static ssize_t prn_store_font(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	unsigned long power;
	
	rc = strict_strtoul(buf, 0, &power);
	if (rc)
		return rc;

	rc = -ENXIO;
	if (power<2 ) 
	{
	    PrnFontSet(font_list[power]);
		rc = count;
	}

	return rc;
}

static ssize_t prn_show_dot_line(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", prn_dot_lines);
}

static ssize_t prn_store_dot_line(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	unsigned long power;

	rc = strict_strtoul(buf, 0, &power);
	if (rc)
		return rc;

	rc = -ENXIO;
	if (power<10 && power>0) 
	{
		prn_dot_lines = (unsigned short)power;
		rc = count;
	}

	return rc;
}
*/
/*
static const char *prn_status[STATUS_MAX]=
{
	"IDLE",			
	"BUSY",
	"SUSPEND",	
	"SUSPEND_NOPAPER",
	"SUSPEND_HOT",	
	"NOPAPER",
};
*/
static int change_paper_status(int status)
{
	int ret=0;
	
	//if(status==	STATUS_SUSPEND_NOPAPER||status==STATUS_NOPAPER)
	{
//		enable_pwr_33();
//		DelayMs(10);
	
		if(s_CheckPaper() != PRN_PAPEROUT)
			ret=1;	
//		disable_pwr_33();	
	}
	
	return ret;
}

static inline void change_suspend2nomal(void)
{
//	if(PrnIsBufPrintted())
		PrnStatus(STATUS_IDLE);
//	else
//		PrnStatus(STATUS_SUSPEND);		
}
	

static ssize_t prn_show_state(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	unsigned short status = PrnStatus(-1);	

	if(status!=STATUS_BUSY)
	{
		int val;

	enable_pwr_33();
	DelayMs(10);
	
		val=s_ReadHeadTemp();
		if(val>=LIMIT_TEMP)
			PrnStatus(STATUS_SUSPEND_HOT);
		else if(status==STATUS_SUSPEND_HOT&&val<LIMIT_RESTART_TEMP)
			change_suspend2nomal();
		
		
		val=s_ReadVoltage();
		if(val<=LIMIT_VOL)
			PrnStatus(STATUS_SUSPEND_LOWLEVEL);
		else if(status==STATUS_SUSPEND_LOWLEVEL&&val>=LIMIT_RESTART_VOL)
			change_suspend2nomal();
		
		val=change_paper_status(status);
		if(val&&(status==	STATUS_SUSPEND_NOPAPER||status==STATUS_NOPAPER))
			change_suspend2nomal();
		else if(val==0)  //nopaper
			PrnStatus(STATUS_NOPAPER);

	disable_pwr_33();
	
		status = PrnStatus(-1);
	}
	
	return sprintf(buf, "%d\n", status);
/*	return sprintf(buf, "%s\n", prn_status[state]);*/
}

static ssize_t prn_store_state(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	unsigned long power;
	unsigned short status;

	rc = strict_strtoul(buf, 0, &power);
#ifdef DEBUG_PRN
	printk("Cmd: %d\n", (int)power);
#endif
	if (rc)
		return rc;

	rc = -ENXIO;

	if (power>STATE_MIN&&power<STATE_MAX) 
	{
		status = PrnStatus(-1);
		switch(power)
		{
			case STATE_START:
	//		case STATE_RESUME:				
				if ((status!=STATUS_BUSY)
					&&!work_pending(&pprn->motor_work)) {
					if(power==STATE_START)ResetPrnPtr();
					PrnState(STATE_START);		/*hupw:20121101 fix 0000540*/			
					queue_work(pprn->motor_workqueue, &pprn->motor_work);
				}
				break;
			case STATE_RESET:
			case STATE_STOP:
				if (status==STATUS_BUSY||work_pending(&pprn->motor_work)) 				
					PrnState(STATE_STOP);
				else
				{
					if(power==STATE_RESET)	PrnClrDotBuf();					
					PrnStatus(STATUS_IDLE);
				}
				break;				
			case STATE_SUSPEND:
//				if (status==STATUS_BUSY||work_pending(&pprn->motor_work)) 				
//					PrnState(STATE_SUSPEND);		
			default:;			
		}
		rc=count;		
	}

	return rc;
}

static ssize_t prn_show_block_div(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", pprn->block_div);
}

static ssize_t prn_store_block_div(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	unsigned long power;

	rc = strict_strtoul(buf, 0, &power);
#ifdef DEBUG_PRN
	printk("DIV: %d\n", (int)power);
#endif	
	if (rc)
		return rc;

	rc = -ENXIO;

	if (power<=MAX_SPLIT_DIV) 
	{
		pprn->block_div = (unsigned short)power;
		rc = count;
	}

	return rc;
}
static ssize_t prn_show_gray_level(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", pprn->gray_level);
}

static ssize_t prn_store_gray_level(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	unsigned long power;

	rc = strict_strtoul(buf, 0, &power);
#ifdef DEBUG_PRN
	printk("Level: %d\n", (int)power);
#endif	
	if (rc)
		return rc;

	rc = -ENXIO;
	if( (power>=0 && power<5) ||(power>=50&&power<=500))
	{
		PrnSetGray((int)power);
		rc = count;
	}

	return rc;
}


static const char *tag_list[MAX_TAGS]={
//	"SYSDEF_TEXT"
	"VGA8x16_TEXT",
	"VGA8x8_TEXT",	
	"GRAPHY",		
	"FEEDBACK"
};

static ssize_t prn_show_tag(struct device *dev,
		struct device_attribute *attr,char *buf)
{

	if(pprn!=NULL){
		return sprintf(buf, "%s\n", tag_list[pprn->wr_tag]);
	}

	return sprintf(buf, "%s\n","NOPRINTER");
}

static ssize_t prn_store_tag(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	unsigned long power;
	
	rc = strict_strtoul(buf, 0, &power);
#ifdef DEBUG_PRN
//	printk("TAG: %d\n", (int)power);
#endif	
	if (rc)
		return rc;

	rc = -ENXIO;
	if (power<MAX_TAGS) 
	{
	    pprn->wr_tag=power ;
		rc = count;
	}

	return rc;
}
static struct device_attribute prn_device_attributes[] = 
{
//	__ATTR(font, 0644, prn_show_font, prn_store_font),	
//	__ATTR(dot_line, 0644, prn_show_dot_line, prn_store_dot_line),
	__ATTR(gray, 0666, prn_show_gray_level,  prn_store_gray_level),
	__ATTR(div, 0666, prn_show_block_div, prn_store_block_div),
	__ATTR(tag, 0666, prn_show_tag, prn_store_tag),	
	__ATTR(state, 0666, prn_show_state, prn_store_state),		
	__ATTR(config, 0444, prn_show_config, NULL),	
	__ATTR(temperature, 0444, prn_show_temperature, NULL),		
	__ATTR_NULL,
};

//=============================================================================
static int prn_open(struct inode* i,struct file* f)
{
	if (pprn->open) 
	{
		printk("Printer is busy, can't be re-openned!\n");		
		return -EBUSY;
	}

	PrnInit();	
    PrnClrDotBuf();

	pprn->open=1;
   return 0;
}

static int prn_close(struct inode* i,struct file* f)
{
	if(pprn->open)
	{
		printk("Printer is closed!\n");			
		pprn->open=0;
		return 0;
	}
	
	return -1;
}

static int prn_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char status= PrnStatus(-1);

	if (copy_to_user(buf, &status, 1))
		return -EFAULT;
	
	return 1;
}


//char prn_write_buf[5000];
static int prn_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned short status; 
	int ret = -EFAULT;
	char *mybuf=NULL;
	
	status = PrnStatus(-1);	
	if(status==STATUS_BUSY)return -EBUSY;		

	if(pprn->wr_tag==GRAPHY)
	{
//		printk("GRAPHY \n");
		if(count>(MAX_DOT_LINE*PRN_LBYTES))
			count=(MAX_DOT_LINE*PRN_LBYTES);
	
		ret=PrnGetDotBuf(buf, (int)count);
		
	/*	if (!work_pending(&pprn->motor_work)) {
			queue_work(pprn->motor_workqueue, &pprn->motor_work);
		}
	*/	
	//	PrnStatus(STATUS_READY);
	}
	else if(pprn->wr_tag==VGA8x16_TEXT||pprn->wr_tag==VGA8x8_TEXT)
	{
		mybuf= kmalloc(count, GFP_KERNEL);
		if(!mybuf)
			return -ENOMEM;

		if(copy_from_user((char *)mybuf, (char *)buf, count))
		{
   	       kfree(mybuf);
			return (-EFAULT);
		}
	//	printk("TEXT: len %d\n", count );			
		mybuf[count]=0;		
   // 	PrnClrDotBuf();	  //
	#if 0 //def DEBUG_PRN	
		sld=0;
	#endif			    
		PrnFontSet((char *)font_list[pprn->wr_tag]);
		ret = s_PrnStr(mybuf);
	    kfree(mybuf);
			
	/*	if (!work_pending(&pprn->motor_work)) {
			queue_work(pprn->motor_workqueue, &pprn->motor_work);
	*/
	//	PrnStatus(STATE_READY);
	}
	else if(pprn->wr_tag==FEEDBACK)
	{
	//		printk("Feed:  %d\n",(int)*((int *)buf) );			

		ret=PrnGetFeedback(buf, sizeof(int));					
	}
	
	return ret;
}

static unsigned int prn_poll(struct file *file, poll_table *wait)
{
	unsigned int ret =0;

	poll_wait(file, &pprn->wq, wait);

	mutex_lock(&pprn->mutex);
	if (pprn->status != STATUS_BUSY)
		ret |= (POLLOUT | POLLWRNORM);
	mutex_unlock(&pprn->mutex);

	return ret;
}

//PRT_PD_N should be set 1 when power on 

static int prn_ioctl(struct inode *inode, struct file *flip, unsigned int cmd, unsigned long arg)
{
//	int ret = 0;
    //int err = 0;

	if(!pprn) return -ENOTTY;
	
    if(_IOC_TYPE(cmd) != PRN_IOC_MAGIC)
        return -ENOTTY;
    if(_IOC_NR(cmd) > PRN_IOC_MAXNR)
        return -ENOTTY;

//	printk("ioctl\n");

    switch(cmd)
    {
		case PRN_INIT:
			s_PrnInit();
		break;
		case PRN_START:
			//s_PrnStr("\n");
			//s_PrnStr("\n");
			if (!work_pending(&pprn->motor_work)) 
					queue_work(pprn->motor_workqueue, &pprn->motor_work);
			printk("prn start\n");
		break;

		case PRN_TEST_PD_ON:
			set_MTDEF();
		break;
		case PRN_TEST_PD_OFF:
			clr_MTDEF();
		break;

		case PRN_TEST_GET_FAT:
//			*(unsigned long *)arg = get_FAT();
      		*(unsigned long *)arg = get_status_temperature();
			break;

		case PRN_TEST_LAT_ON:
			enable_LAT();
		break;
		case PRN_TEST_LAT_OFF:
			disable_LAT();
		break;

		case PRN_TEST_GET_PAPER:
			*(unsigned long *)arg = get_status_paper();
		break;

		case PRN_TEST_A:
			MOTOR_1A_HIGH();
			DelayMs(10);
			MOTOR_1A_LOW();
		break;
		case PRN_TEST_A_MINUS:
			MOTOR_2A_HIGH();
			DelayMs(10);
			MOTOR_2A_LOW();
		break;
		case PRN_TEST_B:
			MOTOR_1B_HIGH();
			DelayMs(10);
			MOTOR_1B_LOW();
		break;
		case PRN_TEST_B_MINUS:
			MOTOR_2B_HIGH();
			DelayMs(10);
			MOTOR_2B_LOW();
		break;

		case PRN_TEST_POWER_ON:
			printk("%s:%d:POWER ON", __func__, __LINE__);
			enable_pwr_t();
		break;
		case PRN_TEST_POWER_OFF:
			printk("POWER OFF");
			disable_pwr_t();
		break;

		case PRN_TEST_STB_ON:
			enable_STB0();
			enable_STB1();
			enable_STB2();
		break;
		case PRN_TEST_STB_OFF:
			disable_STB0();
			disable_STB1();
			disable_STB2();
		break;

		case PRN_TEST_SPI:
			printk("n=%d\n", (int)arg);
			test_spi_data_count((int)arg);
//			PrnFontSet(arg, 1);
		break;

		case PRN_TEST_MOTOR4:
			printk("n=%d\n", (int)arg);
			s_Motor4Step(arg);
		break;
		case PRN_TEST_MOTOR8:
			printk("n=%d\n", (int)arg);
			s_Motor8Step(arg);
		break;
   case PRN_TEST_5V_ON:   
//	 		printk("j2u:%ld -> %ld  \n",MAX_JIFFY_OFFSET, jiffies_to_usecs(MAX_JIFFY_OFFSET));
//	 		printk("u2j:%d -> %ld  \n", 1, usecs_to_jiffies(1));
//	 		printk("u2j:%d -> %ld  \n", 1000, usecs_to_jiffies(1000));
//	 		printk("u2j:%d -> %ld  \n", 1000000, usecs_to_jiffies(1000000));		
    		printk("%s:%d:5v POWER ON", __func__, __LINE__);
			enable_pwr_33();
			enable_pwr_5v();
		break;
   case PRN_TEST_5V_OFF: 
    		printk("5v POWER OFF");
			disable_pwr_33();
			disable_pwr_5v();
		break;
	case PRN_IOC_SETTAG:
		{
			int tag = (int)arg;
			printk("prn_tag=%d\n", tag);
			if(tag>=MIN_TAGS&&tag<MAX_TAGS)
				pprn->wr_tag=tag;
		}
		break;	
	case PRN_GET_DOTLINE:
		{
			*(int*)arg = PrnGetDotLine();
		}
		break;		
    default:
        break;
    }
    return 0;
}



struct file_operations prn_fops =
{
    .owner=THIS_MODULE,
    .open=prn_open,
    .release=prn_close,
    .ioctl=prn_ioctl,
    .read=prn_read,
    .write=prn_write,
	.poll=prn_poll,
};
/*=====================================================*/
#ifdef PRN_IO_REQUEST
static void request_prnIO(struct PrnIOStruct *prnIO)
{
	prnIO->outp_PRT_LAT=get_pax_gpio_num(PIN_PRT_LAT);
    gpio_request(prnIO->outp_PRT_LAT, PIN_PRT_LAT);

	prnIO->outp_PRT_STB=get_pax_gpio_num(PIN_PRT_STB14);
    gpio_request(prnIO->outp_PRT_STB, PIN_PRT_STB14);

	prnIO->outp_PRT_MTAP=get_pax_gpio_num(PIN_PRT_MTAP);
    gpio_request(prnIO->outp_PRT_MTAP, PIN_PRT_MTAP);
	
//	int  outp_PRT_MTAN; 
	prnIO->outp_PRT_MTAN=get_pax_gpio_num(PIN_PRT_MTAN);
    gpio_request(prnIO->outp_PRT_MTAN, PIN_PRT_MTAN);

//	int  outp_PRT_MTBP;
	prnIO->outp_PRT_MTBP=get_pax_gpio_num(PIN_PRT_MTBP);
    gpio_request(prnIO->outp_PRT_MTBP, PIN_PRT_MTBP);
	
//	int  outp_PRT_MTBN;
	prnIO->outp_PRT_MTBN=get_pax_gpio_num(PIN_PRT_MTBN);
    gpio_request(prnIO->outp_PRT_MTBN, PIN_PRT_MTBN);

//	int  outp_PRT_7VON;
	prnIO->outp_PRT_7VON=get_pax_gpio_num(PIN_PRT_7VON);
    gpio_request(prnIO->outp_PRT_7VON, PIN_PRT_7VON);
	
//	int  outp_PRT_3VON ;
	prnIO->outp_PRT_3VON=get_pax_gpio_num(PIN_PRT_3VON);
    gpio_request(prnIO->outp_PRT_3VON, PIN_PRT_3VON);
	
//	int  outp_PRT_PDN ;
	prnIO->outp_PRT_PDN=get_pax_gpio_num(PIN_PRT_PDN);
    gpio_request(prnIO->outp_PRT_PDN, PIN_PRT_PDN);

//	int  inp_PRT_TEMP;
	prnIO->inp_PRT_TEMP=get_pax_gpio_num(PIN_PRT_TEMP);
    gpio_request(prnIO->inp_PRT_TEMP, PIN_PRT_TEMP);
	
//	int  inp_PRT_POUT;
	prnIO->inp_PRT_POUT=get_pax_gpio_num(PIN_PRT_POUT);
    gpio_request(prnIO->inp_PRT_POUT, PIN_PRT_POUT);	
};

static void free_prnIO(struct PrnIOStruct *prnIO)
{
    gpio_free(prnIO->outp_PRT_LAT);

    gpio_free(prnIO->outp_PRT_STB);

    gpio_free(prnIO->outp_PRT_MTAP);
	
//	int  outp_PRT_MTAN; 
    gpio_free(prnIO->outp_PRT_MTAN);

//	int  outp_PRT_MTBP;
    gpio_free(prnIO->outp_PRT_MTBP);
	
//	int  outp_PRT_MTBN;
    gpio_free(prnIO->outp_PRT_MTBN);

//	int  outp_PRT_7VON;
    gpio_free(prnIO->outp_PRT_7VON);
	
//	int  outp_PRT_3VON ;
    gpio_free(prnIO->outp_PRT_3VON);
	
//	int  outp_PRT_PDN ;
    gpio_free(prnIO->outp_PRT_PDN);

//	int  inp_PRT_TEMP;
    gpio_free(prnIO->inp_PRT_TEMP);
	
//	int  inp_PRT_POUT;
    gpio_free(prnIO->inp_PRT_POUT);	
};
#endif

extern void  _init_prn_timer(void);

static int spi_printer_probe(struct spi_device *spi) 
{
	int err;

	g_prn_spi_rdata = kmalloc(sizeof(u8)*SPI_BUF_LEN, GFP_KERNEL);
	g_prn_spi_wdata = kmalloc(sizeof(u8)*SPI_BUF_LEN, GFP_KERNEL);
	if(!g_prn_spi_rdata||!g_prn_spi_wdata)
	{
		err = -ENOMEM;
		goto err_free_mem;
	}
	
/*	//1.register dev*/
	paxprn_major = register_chrdev(0, PRN_DEVNAME, &prn_fops);
	if (paxprn_major < 0)
	{
		printk(KERN_ERR  "Unable to register lprn as a char device\n");
		goto err_free_mem;
	}

	paxprn_class = class_create(THIS_MODULE, PRN_DEVNAME);
	if (IS_ERR(paxprn_class))
	{
		printk(KERN_ERR "Unable to create class for Mxc Ipu\n");
		goto err_unregister_dev;
	}
	
	paxprn_class->dev_attrs = prn_device_attributes;
	pprn->dev=device_create(paxprn_class, NULL, MKDEV(paxprn_major, 0), NULL, "lprn");
	if (IS_ERR(pprn->dev)) {
			printk("Unable to create device for lprn0; errno = %ld\n", 
				PTR_ERR(pprn->dev));
			goto err_class_destory;
	}
	
	printk("[%s][%d] device create lprn0\n", __func__, __LINE__);

    spi_printer_setup(spi, 8);
	spi_printer = spi;	

#ifdef PRN_IO_REQUEST
	request_prnIO(& (pprn->prnIO));
#endif

	s_PrnInit();
    //PrnInit();
   // PrnClrDotBuf();
    PrnSetGray(300);
	_init_prn_timer();

/* irq penging work */
	INIT_WORK(&(pprn->motor_work), s_PrnLooper);
/* create work queue */
	pprn->motor_workqueue = create_singlethread_workqueue(PRN_DEVNAME);

/* irq penging work */
//INIT_WORK(&paxprinter->pen_heat_event_work, s_HeatTimerWork);
/* create work queue */
//paxprinter->heat_workqueue = create_singlethread_workqueue("heat");
	return 0;

err_class_destory:
	class_destroy(paxprn_class);
err_unregister_dev:
	unregister_chrdev(paxprn_major, PRN_DEVNAME);
err_free_mem:
	kfree(g_prn_spi_rdata);
	kfree(g_prn_spi_wdata);
	
	return err;
}
static int spi_printer_remove(struct spi_device *spi) 
{
/*	prn_context *p;*/

	if(!pprn)return 0;
/*
	remove all the context

	list_for_each_entry(p, pprn->context, list) 
	{
		if(p->data)kfree(p->data);
	}
*/
#ifdef PRN_IO_REQUEST
	free_prnIO(& (pprn->prnIO));
#endif

	device_destroy(paxprn_class, MKDEV(paxprn_major, 0));
	class_destroy(paxprn_class);
	unregister_chrdev(paxprn_major, PRN_DEVNAME);
	
	kfree(g_prn_spi_rdata);
	kfree(g_prn_spi_wdata);
	return 0;
}

static int	prn_suspend(struct spi_device *spi, pm_message_t mesg)
{
	unsigned short status;

	printk("===printer suspend===\n");

	status = PrnStatus(-1);
	if (status==STATUS_BUSY||work_pending(&pprn->motor_work))
	{				
		printk("stop printer\n");

		PrnState(STATE_STOP);
	}
	return 0;
}


static void	prn_shutdown(struct spi_device *spi)
{
		unsigned short status;

//	printk("===printer shutdown===\n");

	status = PrnStatus(-1);
	if (status==STATUS_BUSY||work_pending(&pprn->motor_work))
	{				
		printk("printer shutdown\n");

		PrnState(STATE_STOP);
	}
}

static struct spi_driver spi_printer_driver = { 
	.probe = spi_printer_probe, 
	.remove = spi_printer_remove, 
	.driver = { 
		.name = "spi_printer",
		.owner	= THIS_MODULE, 
  	}, 
  	.shutdown=prn_shutdown,
	.suspend = NULL, //prn_suspend,
	.resume = NULL,
}; 



static int __init prn_printer_init(void)
{
	spi_register_driver(&spi_printer_driver); 
	return 0;
}

static void prn_printer_exit(void)
{
	spi_unregister_driver(&spi_printer_driver);
}


module_init(prn_printer_init);
module_exit(prn_printer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(" <.com>");
MODULE_DESCRIPTION("Backlight Lowlevel Control Abstraction");



