#define _POSIX_C_SOURCE 200112L
#include "threads.h"
#include "ring_buffer.h"

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <poll.h>

static void add_us(struct timespec *t, long us)
{
    t->tv_nsec += us * 1000;
    while (t->tv_nsec >= 1000000000L)
    {
        t->tv_nsec -= 1000000000L;
        t->tv_sec++;
    }
}

void *snsrThrdFunc(void *arg)
{
    ring_buffer_t *rb = (ring_buffer_t *)arg;

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    uint32_t tick = 0;

    /* ---------- GPIO setup (event-driven) ---------- */
    int gpio_fd = open("/dev/gpiochip0", O_RDONLY);
    if (gpio_fd < 0)
    {
        perror("gpiochip open");
        return NULL;
    }

    struct gpioevent_request btn_req = {
        .lineoffset = 0, /* BUTTON line */
        .handleflags = GPIOHANDLE_REQUEST_INPUT,
        .eventflags = GPIOEVENT_REQUEST_BOTH_EDGES,
        .consumer_label = "btn"};

    struct gpioevent_request sw_req = {
        .lineoffset = 1, /* SWITCH line */
        .handleflags = GPIOHANDLE_REQUEST_INPUT,
        .eventflags = GPIOEVENT_REQUEST_BOTH_EDGES,
        .consumer_label = "sw"};

    if (ioctl(gpio_fd, GPIO_GET_LINEEVENT_IOCTL, &btn_req) < 0 ||
        ioctl(gpio_fd, GPIO_GET_LINEEVENT_IOCTL, &sw_req) < 0)
    {
        perror("GPIO event request");
        close(gpio_fd);
        return NULL;
    }

    struct pollfd pfd[2] = {
        {.fd = btn_req.fd, .events = POLLIN},
        {.fd = sw_req.fd, .events = POLLIN}};

    while (1)
    {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        /* -------- PT100: 10 Hz (every 100 ms) -------- */
        if ((tick % 1000U) == 0U)
        {
            sample_t s;
            s.sensorId = SENSOR_PT100;
            s.timestampUs =
                (uint64_t)next.tv_sec * 1000000ULL +
                (uint64_t)next.tv_nsec / 1000ULL;
            s.value = 100; /* dummy temperature */

            if (ring_buffer_push(rb, &s) != 0)
            {
            }
        }

        /* -------- SPI ADC: 1 kHz (every 1 ms) -------- */
        if ((tick % 10U) == 0U)
        {
            sample_t s;
            s.sensorId = SENSOR_SPI_ADC;
            s.timestampUs =
                (uint64_t)next.tv_sec * 1000000ULL +
                (uint64_t)next.tv_nsec / 1000ULL;
            s.value = (int32_t)tick;

            if (ring_buffer_push(rb, &s) != 0)
            {
            }
        }

        /* -------- PL ADC: 5 kHz (every 200 µs) -------- */
        if ((tick % 2U) == 0U)
        {
            sample_t s;
            s.sensorId = SENSOR_PL_ADC;
            s.timestampUs =
                (uint64_t)next.tv_sec * 1000000ULL +
                (uint64_t)next.tv_nsec / 1000ULL;
            s.value = (int32_t)(tick * 2U);

            if (ring_buffer_push(rb, &s) != 0)
            {
            }
        }

        /*  GPIO events */
        int ret = poll(pfd, 2, 0); /* zero timeout */
        if (ret > 0)
        {
            struct gpioevent_data ev;

            /* Button */
            if (pfd[0].revents & POLLIN)
            {
                read(btn_req.fd, &ev, sizeof(ev));

                sample_t s;
                s.sensorId = SENSOR_GPIO_BTN;
                s.timestampUs = ev.timestamp / 1000ULL; /* ns → µs */
                s.value = (ev.id == GPIOEVENT_EVENT_RISING_EDGE);

                if (ring_buffer_push(rb, &s) != 0)
                {
                }
            }

            /* Switch */
            if (pfd[1].revents & POLLIN)
            {
                read(sw_req.fd, &ev, sizeof(ev));

                sample_t s;
                s.sensorId = SENSOR_GPIO_SW;
                s.timestampUs = ev.timestamp / 1000ULL;
                s.value = (ev.id == GPIOEVENT_EVENT_RISING_EDGE);

                if (ring_buffer_push(rb, &s) != 0)
                {
                }
            }
        }
        tick++;
        add_us(&next, BASE_TICK_US);
    }
    return NULL;
}

void *ntwrkThrdFunc(void *arg)
{
    ring_buffer_t *rb = (ring_buffer_t *)arg;
    sample_t s;

    while (1)
    {
        if (ring_buffer_pop(rb, &s) == 0)
        {
        }
        else
        {
            usleep(1000);
        }
    }
    return NULL;
}
