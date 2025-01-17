#include <string.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"

#include "orchard-app.h"
#include "led.h"
#include "orchard-ui.h"
#include "images.h"
#include "fontlist.h"

#include "userconfig.h"
#include "gfx.h"

#include "ides_gfx.h"
#include "unlocks.h"

#include "src/gdriver/gdriver.h"
#include "src/ginput/ginput_driver_mouse.h"

#include "nrf52i2s_lld.h"
#include "ble_lld.h"

/* the amount that we increase or decrease brightness by */
#define LED_BRIGHT_STEP 10

// GHandles
typedef struct _SetupHandles {
  GHandle ghCheckSound;
  GHandle ghCheckAirplane;
  GHandle ghCheckRotate;
  GHandle ghLabel1;
  GHandle ghLabelPattern;
  GHandle ghButtonPatDn;
  GHandle ghButtonPatUp;
  GHandle ghLabel4;
  GHandle ghButtonDimUp;
  GHandle ghButtonDimDn;
  GHandle ghButtonOK;
  GHandle ghButtonCalibrate;
  GHandle ghButtonSetName;
  GListener glSetup;
} SetupHandles;

static uint32_t last_ui_time = 0;
extern char *fxlist[];

static font_t fontSM;
static font_t fontXS;
/* prototypes */
static void nextLedPattern(uint8_t);
static void prevLedPattern(uint8_t);
static void nextLedBright(void);
static void prevLedBright(void);

static void draw_setup_buttons(SetupHandles * p) {
  userconfig *config = getConfig();
  GWidgetInit wi;

  gwinSetDefaultFont(fontSM);
  gwinWidgetClearInit(&wi);

  // create checkbox widget: ghCheckSound
  wi.g.show = TRUE;
  wi.g.x = 10;
  wi.g.y = 120;
  wi.g.width = 180;
  wi.g.height = 20;
  wi.text = "Sounds ON";
  wi.customDraw = gwinCheckboxDraw_CheckOnLeft;
  wi.customParam = 0;
  wi.customStyle = 0;
  p->ghCheckSound = gwinCheckboxCreate(0, &wi);
  gwinCheckboxCheck(p->ghCheckSound, config->sound_enabled);

  // create checkbox widget: ghCheckAirplane
  wi.g.show = TRUE;
  wi.g.y = 150;
  wi.text = "Airplane Mode";
  wi.customDraw = gwinCheckboxDraw_CheckOnLeft;
  p->ghCheckAirplane = gwinCheckboxCreate(0, &wi);
  gwinCheckboxCheck(p->ghCheckAirplane, config->airplane_mode);

  // create checkbox widget: ghCheckRotate
  wi.g.show = TRUE;
  wi.g.y = 180;
  wi.text = "Rot. LED Sign";
  wi.customDraw = gwinCheckboxDraw_CheckOnLeft;
  p->ghCheckRotate = gwinCheckboxCreate(0, &wi);
  gwinCheckboxCheck(p->ghCheckRotate, config->rotate);

  // Create label widget: ghLabel1
  wi.g.show = TRUE;
  wi.g.x = 10;
  wi.g.y = 10;
  wi.g.width = 180;
  wi.g.height = 20;
  wi.text = "LED Pattern";
  wi.customDraw = gwinLabelDrawJustifiedLeft;
  p->ghLabel1 = gwinLabelCreate(0, &wi);
  gwinLabelSetBorder(p->ghLabel1, FALSE);

  // Create label widget: ghLabelPattern
  wi.g.y = 30;
  wi.text = "";
  p->ghLabelPattern = gwinLabelCreate(0, &wi);
  gwinLabelSetBorder(p->ghLabelPattern, FALSE);

  // create button widget: ghButtonPatDn
  wi.g.show = TRUE;
  wi.g.x = 260;
  wi.g.y = 10;
  wi.g.width = 50;
  wi.g.height = 40;
  wi.text = "->";
  wi.customDraw = gwinButtonDraw_Normal;
  wi.customParam = 0;
  wi.customStyle = &DarkPurpleFilledStyle;
  p->ghButtonPatDn = gwinButtonCreate(0, &wi);

  // create button widget: ghButtonPatUp
  wi.g.x = 200;
  wi.g.y = 10;
  wi.g.width = 50;
  wi.g.height = 40;
  wi.text = "<-";
  wi.customDraw = gwinButtonDraw_Normal;
  wi.customStyle = &DarkPurpleFilledStyle;
  p->ghButtonPatUp = gwinButtonCreate(0, &wi);

  // Create label widget: ghLabel4
  wi.g.x = 10;
  wi.g.y = 60;
  wi.g.width = 180;
  wi.g.height = 20;
  wi.text = "LED Brightness";
  wi.customDraw = gwinLabelDrawJustifiedLeft;
  wi.customStyle = 0;
  p->ghLabel4 = gwinLabelCreate(0, &wi);
  gwinLabelSetBorder(p->ghLabel4, FALSE);

  // create button widget: ghButtonDimDn
  wi.g.show = TRUE;
  wi.g.x = 200;
  wi.g.y = 60;
  wi.g.width = 50;
  wi.g.height = 40;
  wi.text = "<-";
  wi.customDraw = gwinButtonDraw_Normal;
  wi.customParam = 0;
  wi.customStyle = &DarkPurpleFilledStyle;
  p->ghButtonDimDn = gwinButtonCreate(0, &wi);

  // create button widget: ghButtonDimUp
  wi.g.x = 260;
  wi.text = "->";
  wi.customStyle = &DarkPurpleFilledStyle;
  p->ghButtonDimUp = gwinButtonCreate(0, &wi);

  // create button widget: ghButtonCalibrate
  wi.g.x = 200;
  wi.g.y = 110;
  wi.g.width = 110;
  wi.g.height = 36;
  wi.text = "Touch Cal";
  p->ghButtonCalibrate = gwinButtonCreate(0, &wi);

  // create button widget: ghButtonSetName
  wi.g.y = 150;
  wi.text = "Set Name";
  p->ghButtonSetName = gwinButtonCreate(0, &wi);

  // create button widget: ghButtonOK
  wi.g.y = 190;
  wi.text = "Save";
  p->ghButtonOK = gwinButtonCreate(0, &wi);

  drawProgressBar(10,80,180,20,255, config->led_brightness, false, false);
}

static uint32_t setup_init(OrchardAppContext *context) {
  (void)context;
  last_ui_time = chVTGetSystemTime();
  return 0;
}

static void setup_start(OrchardAppContext *context) {
  SetupHandles * p;

  fontSM = gdispOpenFont (FONT_SM);
  fontXS = gdispOpenFont (FONT_XS);

  gdispClear (Black);

  p = malloc (sizeof(SetupHandles));
  draw_setup_buttons(p);
  context->priv = p;

  // idle ui timer (10s)
  last_ui_time = chVTGetSystemTime();
  orchardAppTimer(context, 1000000, true);

  geventListenerInit(&p->glSetup);
  gwinAttachListener(&p->glSetup);
  geventRegisterCallback (&p->glSetup, orchardAppUgfxCallback, &p->glSetup);

}

static void nextLedPattern(uint8_t max_led_patterns) {
  userconfig *config = getConfig();
  config->led_pattern++;
  if (config->led_pattern >= max_led_patterns) config->led_pattern = 0;
  ledSetPattern(config->led_pattern);

  if (config->led_pattern == 0) {
    ledStop();
  }
}

static void prevLedPattern(uint8_t max_led_patterns) {
  userconfig *config = getConfig();
  config->led_pattern--;
  if (config->led_pattern == 255) config->led_pattern = max_led_patterns - 1;
  ledSetPattern(config->led_pattern);

  if (config->led_pattern == 0) {
    ledStop();
  }
}

static void prevLedBright() {
  userconfig *config = getConfig();
  int a;

  a = config->led_brightness;
  if ( (a - LED_BRIGHT_STEP) < 0 ) {
    // underflow
    config->led_brightness = 0;
  } else {
    config->led_brightness -= LED_BRIGHT_STEP;
  }

  led_brightness_set(config->led_brightness);
  drawProgressBar(10,80,180,20,255, config->led_brightness, false, false);

}

static void nextLedBright() {
  userconfig *config = getConfig();
  int a;

  a = config->led_brightness;
  if ( (a + LED_BRIGHT_STEP) > 255 ) {
    // overflow
    config->led_brightness = 255;
  } else {
    config->led_brightness += LED_BRIGHT_STEP;
  }

  led_brightness_set(config->led_brightness);
  drawProgressBar(10,80,180,20,255, config->led_brightness, false, false);
}

static void setup_event(OrchardAppContext *context,
	const OrchardAppEvent *event) {
  GEvent * pe;
  userconfig *config = getConfig();
  SetupHandles * p;
  uint8_t max_led_patterns;
  GMouse * m;

  p = context->priv;

  /* handle events */
  if (event->type == radioEvent) {
    /* Ignore radio events */
    return;
  }

  // idle timeout
  if (event->type == timerEvent) {
    if( (chVTGetSystemTime() - last_ui_time) > (UI_IDLE_TIME * 1000) && last_ui_time != 0) {

      orchardAppRun(orchardAppByName("Badge"));
    }
    return;
  }

  if ( config->unlocks & UL_LEDS ) {
    max_led_patterns = LED_PATTERNS_FULL;
  } else {
    max_led_patterns = LED_PATTERNS_LIMITED;
  }

  if (event->type == keyEvent && event->key.flags == keyRelease) {
    last_ui_time = chVTGetSystemTime();

    switch(event->key.code) {
      case keyALeft:
        prevLedBright();
        break;
      case keyARight:
        nextLedBright();
        break;
      case keyAUp:
        prevLedPattern(max_led_patterns);
        break;
      case keyADown:
        nextLedPattern(max_led_patterns);
        break;
      default:
        break; // all other keys ignored
    }
  }
  if (event->type == keyEvent && event->key.flags == keyPress) {
    last_ui_time = chVTGetSystemTime();
    i2sPlay("sound/click.snd");
  }

  if (event->type == ugfxEvent) {
    pe = event->ugfx.pEvent;
    last_ui_time = chVTGetSystemTime();
    i2sPlay("sound/click.snd");
    switch(pe->type) {

    case GEVENT_GWIN_CHECKBOX:
      /* handle checkbox state changes */
      if (((GEventGWinCheckbox*)pe)->gwin == p->ghCheckSound) {
        config->sound_enabled = ((GEventGWinCheckbox*)pe)->isChecked;
        if (config->sound_enabled)
          i2sEnabled = TRUE;
        else
          i2sEnabled = FALSE;
      }
      if (((GEventGWinCheckbox*)pe)->gwin == p->ghCheckAirplane) {
        config->airplane_mode = ((GEventGWinCheckbox*)pe)->isChecked;
        if (config->airplane_mode)
          bleDisable ();
        else
          bleEnable ();
      }
      if (((GEventGWinCheckbox*)pe)->gwin == p->ghCheckRotate) {
        config->rotate = ((GEventGWinCheckbox*)pe)->isChecked;
      }
      break;
    case GEVENT_GWIN_BUTTON:
      if (((GEventGWinButton*)pe)->gwin == p->ghButtonOK) {
          configSave(config);
          orchardAppExit();
          return;
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghButtonCalibrate) {
          /*
           * We need to allow some time for the widget redaw
           * for the "TOUCH CAL" button to complete before we
           * start the calibrator, otherwise the redraw might
           * corrupt the display.
           */
          chThdSleepMilliseconds (200);

          /* Detach the event handler from the mouse */
          geventDetachSource (&p->glSetup, NULL);
          geventRegisterCallback (&p->glSetup, NULL, NULL);
          /* Run the calibration GUI */
          (void)ginputCalibrateMouse (0);

          /* Save the calibration data */
          m = (GMouse*)gdriverGetInstance (GDRIVER_TYPE_MOUSE, 0);
          memcpy (&config->touch_data, &m->caldata,
            sizeof(config->touch_data));
          config->touch_data_present = 1;

          configSave(config);
          orchardAppExit();
          return;
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghButtonSetName) {
          configSave(config);
          orchardAppRun(orchardAppByName("Set your name"));
          return;
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghButtonDimDn) {
        prevLedBright();
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghButtonDimUp) {
        nextLedBright();
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghButtonPatDn) {
        nextLedPattern(max_led_patterns);
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghButtonPatUp) {
        prevLedPattern(max_led_patterns);
      }
      break;
    }
  }

  // update ui
  gwinSetText(p->ghLabelPattern, fxlist[config->led_pattern], TRUE);
}

static void setup_exit(OrchardAppContext *context) {
  SetupHandles * p;

  p = context->priv;

  gwinDestroy(p->ghCheckSound);
  gwinDestroy(p->ghCheckAirplane);
  gwinDestroy(p->ghCheckRotate);
  gwinDestroy(p->ghLabel1);
  gwinDestroy(p->ghLabelPattern);
  gwinDestroy(p->ghButtonPatDn);
  gwinDestroy(p->ghButtonPatUp);
  gwinDestroy(p->ghLabel4);
  gwinDestroy(p->ghButtonDimUp);
  gwinDestroy(p->ghButtonDimDn);
  gwinDestroy(p->ghButtonOK);
  gwinDestroy(p->ghButtonCalibrate);
  gwinDestroy(p->ghButtonSetName);

  gdispCloseFont(fontXS);
  gdispCloseFont(fontSM);

  geventDetachSource (&p->glSetup, NULL);
  geventRegisterCallback (&p->glSetup, NULL, NULL);

  free (context->priv);
  context->priv = NULL;

  return;
}

orchard_app("Setup", "icons/wheel.rgb", 0, setup_init, setup_start,
    setup_event, setup_exit, 2);
