/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
#ifndef __DRIVERS_AUDIO_ALC5658_H
#define __DRIVERS_AUDIO_ALC5658_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <tinyara/compiler.h>
#include <tinyara/fs/ioctl.h>
#include <stdint.h>

#ifdef CONFIG_AUDIO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ALC5658_DEFAULT_SAMPRATE	16000
#define ALC5658_DEFAULT_NCHANNELS	2
#define ALC5658_DEFAULT_BPSAMP		16
#define FAIL				0xFFFF
#define         alc5658_givesem(s) sem_post(s)

/* Commonly defined and redefined macros */

#ifndef MIN
#define MIN(a, b)                   (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)                   (((a) > (b)) ? (a) : (b))
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/
/* This should be put under hammer to strip size 
by for status variables */

struct alc5658_dev_s {
	/* We are an audio lower half driver (We are also the upper "half" of
	 * the ALC5658 driver with respect to the board lower half driver).
	 *
	 * Terminology: Our "lower" half audio instances will be called dev for the
	 * publicly visible version and "priv" for the version that only this driver
	 * knows.  From the point of view of this driver, it is the board lower
	 * "half" that is referred to as "lower".
	 */

	struct audio_lowerhalf_s dev;	/* ALC5658 audio lower half (this device) */

	/* Our specific driver data goes here */

	FAR struct alc5658_lower_s *lower;	/* Pointer to the board lower functions */
	FAR struct i2c_dev_s *i2c;	/* I2C driver to use */
	FAR struct i2s_dev_s *i2s;	/* I2S driver to use */
	struct dq_queue_s pendq;	/* Queue of pending buffers to be sent */
	struct dq_queue_s doneq;	/* Queue of sent buffers to be returned */
	sem_t devsem;				/* Protection for both pendq & dev*/

#ifdef ALC5658_USE_FFLOCK_INT
	struct work_s work;			/* Interrupt work */
#endif
	uint16_t samprate;			/* Configured samprate (samples/sec) */
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
	uint16_t balance;			/* Current balance level (b16) */
#endif							/* CONFIG_AUDIO_EXCLUDE_BALANCE */
	uint8_t volume;				/* Current volume level {0..63} */
#endif							/* CONFIG_AUDIO_EXCLUDE_VOLUME */
	uint8_t nchannels;			/* Number of channels (1 or 2) */
	uint8_t bpsamp;				/* Bits per sample (8 or 16) */
	volatile uint8_t inflight;	/* Number of audio buffers in-flight */
#ifdef ALC5658_USE_FFLOCK_INT
	volatile bool locked;		/* FLL is locked */
#endif
	bool running;				/* True: Worker thread is running */
	bool paused;				/* True: Playing is paused */
	bool mute;					/* True: Output is muted */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
	bool terminating;			/* True: Stop requested */
#endif
	bool reserved;				/* True: Device is reserved */
	volatile int result;		/* The result of the last transfer */
	bool inout;					/* True: IN device */
};

/****************************************************************************
 * Private Declarations 
 ****************************************************************************/

#ifdef CONFIG_ALC5658_CLKDEBUG
extern const uint8_t g_sysclk_scaleb1[ALC5658_BCLK_MAXDIV + 1];
extern const uint8_t g_fllratio[ALC5658_NFLLRATIO];
#endif

#if defined(CONFIG_ALC5658_REGDUMP) || defined(CONFIG_ALC5658_CLKDEBUG)
static uint16_t alc5658_readreg(FAR struct alc5658_dev_s *priv, uint16_t regaddr);
#endif
static void alc5658_writereg(FAR struct alc5658_dev_s *priv, uint16_t regaddr, uint16_t regval);
static void alc5658_takesem(sem_t *sem);
static uint16_t alc5658_modifyreg(FAR struct alc5658_dev_s *priv, uint16_t regaddr, uint16_t set, uint16_t clear);

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
static inline uint16_t alc5658_scalevolume(uint16_t volume, b16_t scale);
static void alc5658_setvolume(FAR struct alc5658_dev_s *priv, uint16_t volume, bool mute);
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_TONE
static void alc5658_setbass(FAR struct alc5658_dev_s *priv, uint8_t bass);
static void alc5658_settreble(FAR struct alc5658_dev_s *priv, uint8_t treble);
#endif
static void alc5658_set_i2s_datawidth(FAR struct alc5658_dev_s *priv);
static void alc5658_set_i2s_samplerate(FAR struct alc5658_dev_s *priv);

/* Audio lower half methods (and close friends) */

static int alc5658_getcaps(FAR struct audio_lowerhalf_s *dev, int type, FAR struct audio_caps_s *caps);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int alc5658_configure(FAR struct audio_lowerhalf_s *dev, FAR void *session, FAR const struct audio_caps_s *caps);
#else
static int alc5658_configure(FAR struct audio_lowerhalf_s *dev, FAR const struct audio_caps_s *caps);
#endif
static int alc5658_shutdown(FAR struct audio_lowerhalf_s *dev);

#ifdef CONFIG_AUDIO_MULTI_SESSION
static int alc5658_start(FAR struct audio_lowerhalf_s *dev, FAR void *session);
#else
static int alc5658_start(FAR struct audio_lowerhalf_s *dev);
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int alc5658_stop(FAR struct audio_lowerhalf_s *dev, FAR void *session);
#else
static int alc5658_stop(FAR struct audio_lowerhalf_s *dev);
#endif
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int alc5658_pause(FAR struct audio_lowerhalf_s *dev, FAR void *session);
static int alc5658_resume(FAR struct audio_lowerhalf_s *dev, FAR void *session);
#else
static int alc5658_pause(FAR struct audio_lowerhalf_s *dev);
static int alc5658_resume(FAR struct audio_lowerhalf_s *dev);
#endif
#endif
static int alc5658_enqueuebuffer(FAR struct audio_lowerhalf_s *dev, FAR struct ap_buffer_s *apb);
static int alc5658_cancelbuffer(FAR struct audio_lowerhalf_s *dev, FAR struct ap_buffer_s *apb);
static int alc5658_ioctl(FAR struct audio_lowerhalf_s *dev, int cmd, unsigned long arg);
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int alc5658_reserve(FAR struct audio_lowerhalf_s *dev, FAR void **session);
#else
static int alc5658_reserve(FAR struct audio_lowerhalf_s *dev);
#endif
#ifdef CONFIG_AUDIO_MULTI_SESSION
static int alc5658_release(FAR struct audio_lowerhalf_s *dev, FAR void *session);
#else
static int alc5658_release(FAR struct audio_lowerhalf_s *dev);
#endif

/* Interrupt handling an worker thread */

#ifdef ALC5658_USE_FFLOCK_INT
static void alc5658_interrupt_work(FAR void *arg);
static int alc5658_interrupt(FAR const struct alc5658_lower_s *lower, FAR void *arg);
#endif

/* Initialization */
static void alc5658_audio_output(FAR struct alc5658_dev_s *priv);
static void alc5658_audio_input(FAR struct alc5658_dev_s *priv);
#ifdef ALC5658_USE_FFLOCK_INT
static void alc5658_configure_ints(FAR struct alc5658_dev_s *priv);
#else
#define       alc5658_configure_ints(p)
#endif
static void alc5658_hw_reset(FAR struct alc5658_dev_s *priv);
uint16_t alc5658_readreg(FAR struct alc5658_dev_s *priv, uint16_t regaddr);

#endif							/* CONFIG_AUDIO */
#endif							/* __DRIVERS_AUDIO_ALC5658_H */
