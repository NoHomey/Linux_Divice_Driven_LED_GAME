#include "tlc5947.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <tlc5947_ioctl.h>

int tlc5947_init(struct tlc5947* tlc5947, const uint8_t chips, const uint8_t wiring) {
    tlc5947->chips = chips;
    tlc5947->leds = chips * LIBTLC5947_LEDS;
    tlc5947->length = tlc5947->leds + (tlc5947->leds / 2);
    tlc5947->fd = open(LIBTLC5947_DEVICE, O_WRONLY);
    if(tlc5947->fd == -1) {
        return -errno;
    }
    tlc5947->pwm = (uint16_t*) calloc(tlc5947->leds, sizeof(uint16_t));
    if(!tlc5947->pwm) {
        return -ENOMEM;
    }
    tlc5947->buffer = (unsigned char*) calloc(tlc5947->length, sizeof(unsigned char));
	if(ioctl(tlc5947->fd, TLC5947_ALLOC, tlc5947->length) == -1) {
		return -ENOMEM;
	}
    if(!tlc5947->buffer) {
        return -ENOMEM;
    }
    tlc5947->wiring[0] = Wir_RED(wiring);
    tlc5947->wiring[1] = Wir_GREEN(wiring);
    tlc5947->wiring[2] = Wir_BLUE(wiring);

    return 0;
}

void tlc5947_free(struct tlc5947* tlc5947) {
    free(tlc5947->buffer);
    free(tlc5947->pwm);
	ioctl(tlc5947->fd, TLC5947_FREE);
    close(tlc5947->fd);
}

int tlc5947_setLED(struct tlc5947* tlc5947, const uint16_t led, const uint16_t pwm) {
    if(led >= tlc5947->leds) {
        return -EINVAL;
    }
    if(pwm > LIBTLC5947_MAXPWM) {
        return -EINVAL;
    }
    tlc5947->pwm[led] = pwm;

    return 0;
}

int tlc5947_setRGBLED(struct tlc5947* tlc5947, const uint16_t rgb, const uint16_t* pwm) {
    uint8_t i;
    int result = 0;
    uint16_t led = rgb * 3;
    if(tlc5947->wiring[0] == tlc5947->wiring[1]) {
        return -EPERM;
    }
    for(i = 0; i < 3; ++i) {
        result |= tlc5947_setLED(tlc5947, led + tlc5947->wiring[i], pwm[i]);
    }

    return result;
}

int tlc5947_unsetLED(struct tlc5947* tlc5947, const uint16_t led) {
    return tlc5947_setLED(tlc5947, led, 0);
}

int tlc5947_unsetRGBLED(struct tlc5947* tlc5947, const uint16_t rgb) {
    uint16_t pwm[] = {0, 0, 0};
    return tlc5947_setRGBLED(tlc5947, rgb, pwm);
}

int tlc5947_write(struct tlc5947* tlc5947) {
    int written;
    uint8_t i;
    uint16_t k = 0;
    for(i = 0; i < tlc5947->leds; i += 2) {
        tlc5947->buffer[k++] = tlc5947->pwm[i] & 255;
        tlc5947->buffer[k++] = ((tlc5947->pwm[i] >> 8) & 15) + ((tlc5947->pwm[i + 1] << 4) & 240);
        tlc5947->buffer[k++] = (tlc5947->pwm[i + 1] >> 4) & 255;
    }
    errno = 0;
    written = write(tlc5947->fd, (char*) tlc5947->buffer, tlc5947->length);
    if(written < 0) {
        tlc5947_free(tlc5947);
        return -errno;
    }

    return 0;
}
