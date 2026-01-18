#define _POSIX_C_SOURCE 200112L
#include "threads.h"
#include "ring_buffer.h"

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <poll.h>
#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT_NUMBER 5000
#define MAX_BATCH_SAMPLES 64            //can be increased, it is purposely set low to allow margin
#define MAX_PAYLOAD_BYTES (MAX_BATCH_SAMPLES * sizeof(sample_t))

static inline int64_t timeUs(const struct timespec *t)
{
    return (int64_t)t->tv_sec * 1000000LL +
           (int64_t)t->tv_nsec / 1000LL;
}

static void addUs(struct timespec *t, long us)
{
    t->tv_nsec += us * 1000;
    while (t->tv_nsec >= 1000000000L)
    {
        t->tv_nsec -= 1000000000L;
        t->tv_sec++;
    }
}

static void setRealTime(int priority)
{
    struct sched_param sp;
    sp.sched_priority = priority;

    if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0)
    {
        perror("sched_setscheduler");
    }
}

static void setCpuAffinity(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0)
    {
        perror("sched_setaffinity");
    }
}

void *snsrThrdFunc(void *arg)
{
    setCpuAffinity(0);
    setRealTime(80);

    static jitter_stats_t jitterStats = {
    .min_jitter_us = 1000000,
    .max_jitter_us = -1000000,
    .sum_jitter_us = 0,                         //will overflow if it runs for a long time
    .samples = 0,
    .missed_deadlines = 0};

    ring_buffer_t *rb = (ring_buffer_t *)arg;

    struct timespec next;
    struct timespec now;
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
        clock_gettime(CLOCK_MONOTONIC, &now);

        int64_t expectedUs = timeUs(&next);
        int64_t actualUs = timeUs(&now);
        int64_t jitterUs = actualUs - expectedUs;

        /* Update stats*/
        if (jitterUs < jitterStats.min_jitter_us)
            jitterStats.min_jitter_us = jitterUs;

        if (jitterUs > jitterStats.max_jitter_us)
            jitterStats.max_jitter_us = jitterUs;

        jitterStats.sum_jitter_us += jitterUs;
        jitterStats.samples++;

        /* Missed deadline detection*/
        if (jitterUs > (int64_t)BASE_TICK_US)
            jitterStats.missed_deadlines++;

        /* -------- PT100: 10 Hz (every 100 ms) -------- */
        if ((tick % 1000U) == 0U)
        {
            sample_t s;
            s.sensorId = SENSOR_PT100;
            s.timestampUs =
                (uint64_t)next.tv_sec * 1000000ULL +
                (uint64_t)next.tv_nsec / 1000ULL;
            s.value = 100; /* dummy temperature */

            if (ringBufferPush(rb, &s) != 0)
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

            if (ringBufferPush(rb, &s) != 0)
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

            if (ringBufferPush(rb, &s) != 0)
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

                if (ringBufferPush(rb, &s) != 0)
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

                if (ringBufferPush(rb, &s) != 0)
                {
                }
            }
        }

        if ((jitterStats.samples % 10000ULL) == 0ULL)
        {
            int64_t avg = jitterStats.sum_jitter_us / (int64_t)jitterStats.samples;

            printf("[JITTER] min=%ld us, max=%ld us, avg=%ld us, missed=%lu\n", 
                jitterStats.min_jitter_us, 
                jitterStats.max_jitter_us, 
                avg, 
                jitterStats.missed_deadlines);
        }
        tick++;
        addUs(&next, BASE_TICK_US);
    }
    return NULL;
}

void *ntwrkThrdFunc(void *arg)
{
    setCpuAffinity(1);
    setRealTime(20);

    ring_buffer_t *rb = (ring_buffer_t *)arg;
    sample_t s;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverFd < 0){
        perror("socket");
        return NULL;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_NUMBER),
        .sin_addr.s_addr = INADDR_ANY
    };

    if(bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("bind");
        return NULL;
    }

    if(listen(serverFd, 1) < 0){
        perror("listen");
        return NULL;
    }

    printf("TCP server listening on port %d\n", PORT_NUMBER);

    int clientFd = accept(serverFd, NULL, NULL);
    if(clientFd < 0){
        perror("accept");
        return NULL;
    }

    printf("Client connected...\n");

    uint8_t txBuffer[2 + MAX_PAYLOAD_BYTES];
    sample_t batch[MAX_BATCH_SAMPLES];

    while (1)
    {
        uint32_t count = ringBufferCount(rb);

        /* Drop oldest samples if backlog grows */
        if(count > MAX_BATCH_SAMPLES){
            uint32_t drop = count - MAX_BATCH_SAMPLES;
            while(drop--){
                ringBufferPop(rb, &s);
            }
        }

        size_t n = 0;
        while(n < MAX_BATCH_SAMPLES){
            if(ringBufferPop(rb, &batch[n]) == 0)
                n++;
            else
                break;
        }

        if(n == 0){
            usleep(1000);
            continue;
        }

        uint16_t payloadLen = n * sizeof(sample_t);
        txBuffer[0] = (payloadLen >> 8) & 0xFF;
        txBuffer[1] = payloadLen & 0xFF;

        memcpy(&txBuffer[2], batch, payloadLen);

        ssize_t sent = send(clientFd, txBuffer, payloadLen + 2, 0);
        if(sent <= 0){
            perror("send");
            break;
        }
    }

    /* Terminate client socket and server socket*/
    close(clientFd);
    close(serverFd);
    return NULL;
}
