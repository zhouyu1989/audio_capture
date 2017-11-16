#include "pipe.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include "pipe_util.h"
#include <netinet/in.h>
#include <cutils/sockets.h>
#include <tinyalsa/asoundlib.h>

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

struct wav_header header;
unsigned int card = 0;
unsigned int device = 0;
unsigned int channels = 8;
unsigned int rate = 48000;
unsigned int bits = 32;
unsigned int frames;
unsigned int period_size = 1024;
unsigned int period_count = 4;
enum pcm_format format;
char gPathName[1024] = {0};

pthread_mutex_t g_mutex;
#define RECORD_LOCK() pthread_mutex_lock(&g_mutex);
#define RECORD_UNLOCK() pthread_mutex_unlock(&g_mutex);

//need mutex lock mechanism to protect.
int capturing = 0;
int capFinish = 1;
int error = 0;

static int begin_capture(char *fileName);
 
static unsigned int capture_sample(unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count);

static FILE * openWavFile(char *fileName)
{
    FILE *file;

    file = fopen(fileName, "wb");
    if (!file) {
        fprintf(stderr, "Unable to create file '%s'\n", fileName);
        return NULL;
    }

    return file;
}

static pipe_t* pipe_len = NULL;
static pipe_producer_t* p_len = NULL;
static pipe_consumer_t* c_len = NULL;
static pipe_t* pipe_data = NULL;
static pipe_producer_t* p_data = NULL;
static pipe_consumer_t* c_data = NULL;
static char p_buffer[262411];
static char c_buffer[262411];


static void *capturing_thread(void *data)
{
    //printf("=====: %s====\n", gPathName);
    if(gPathName[0] != 0) {
        begin_capture(gPathName);
    }
    return NULL;
}

static void createCapturingThread(void)
{
    pthread_t thread;

    pthread_create(&thread, NULL, capturing_thread,
            NULL);
}

static void *recording_thread(void *data)
{
    int frames = 0;

    printf("enter recording thread\n");
    frames = capture_sample(card, device, header.num_channels,
                        header.sample_rate, format,
                        period_size, period_count);
    printf("Captured %d frames\n", frames);
    return NULL;
}

static void createRecordingThread(void)
{

    pthread_t thread;

    pthread_create(&thread, NULL, recording_thread,
            NULL);
}

static int begin_capture(char *fileName) {
    FILE *file;
    int len = 0;
    int size = 0;
    printf("enter begin_capture, fileName: %s\n", fileName);
    file = openWavFile(fileName);
    //printf("file = %p \n", file);
    //printf("=====aaaa===\n");
    if(file != NULL) {
        //printf("begin set pipe 1\n");
        pipe_len = pipe_new(sizeof(int), 0);
        p_len = pipe_producer_new(pipe_len);
        c_len = pipe_consumer_new(pipe_len);

        //printf("begin set pipe 2\n");
        pipe_data = pipe_new(sizeof(p_buffer), 0);
        p_data = pipe_producer_new(pipe_data);
        c_data = pipe_consumer_new(pipe_data);

        //start record thread.
        //printf("start recording thread!\n");
        createRecordingThread();

        /* leave enough room for header */
        //fseek(file, sizeof(struct wav_header), SEEK_SET);

        //TODO: push/pop mechanism to get recording thread's data.
        while(pipe_pop(c_len, &len, 1)) {
            //printf("len = %d\n", len);
            if(len == 0)
                break;

            size += len;

            pipe_pop(c_data, c_buffer, 1);
            fwrite(c_buffer, 1, len, file);
        }

        printf("exit capture, size = %d\n", size);
        /* write header now all information is known */
        header.data_sz = frames * header.block_align;
        header.data_sz = size;
        header.riff_sz = header.data_sz + sizeof(header) - 8;
        //fseek(file, 0, SEEK_SET);
        //fwrite(&header, sizeof(struct wav_header), 1, file);
        //fclose(file);
        RECORD_LOCK()
        capFinish = 1;
        RECORD_UNLOCK()

        pipe_producer_free(p_len);
        pipe_consumer_free(c_len);
        pipe_free(pipe_len);

        pipe_producer_free(p_data);
        pipe_consumer_free(c_data);
        pipe_free(pipe_data);

        p_len = NULL;
        c_len = NULL;
        pipe_len = NULL;

        p_data = NULL;
        c_data = NULL;
        pipe_data = NULL;

    } else {
        printf("open fileName: %s failed\n", fileName);
    }

    return 0;
}

static unsigned int capture_sample(unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count)
{
    char *buffer;
    struct pcm *pcm;
    int size;
    int local_capturing = 0;
    struct pcm_config config;
    unsigned int bytes_read = 0;

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    //ROKID open alsa driver
    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        return 0;
    }

    //ROKID get the pcm buffer size.
    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", size);
        free(buffer);
        pcm_close(pcm);
        return 0;
    }

    printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));

    //ROKID get the buffer from kernel space.
    //Dear Zhubo, you can get the raw data from here.

    RECORD_LOCK()
    local_capturing = capturing;
    RECORD_UNLOCK()

    while (local_capturing && !pcm_read(pcm, buffer, size)) {
/*
        if (fwrite(buffer, 1, size, file) != size) {
            fprintf(stderr,"Error capturing sample\n");
            break;
        }
*/

        //TODO push the data into file write thread.
        //printf("begin push 11 the record data into emmc write thread.\n");
        pipe_push(p_len, &size, 1);
        //printf("begin push 22 the record data into emmc write thread.\n");
        memcpy(p_buffer, buffer, size);
        //printf("begin push 33 the record data into emmc write thread.\n");
        pipe_push(p_data, p_buffer, 1);
        //printf("begin push 44 the record data into emmc write thread.\n");

        bytes_read += size;
        RECORD_LOCK()
        local_capturing = capturing;
        RECORD_UNLOCK()
    }

    //notify emmc write thread, recording finish.
    size = 0;
    pipe_push(p_len, &size, 1);

    free(buffer);
    //ROKID close pcm node.
    pcm_close(pcm);
    //ROKID return the total frames length this time.

    return pcm_bytes_to_frames(pcm, bytes_read);
}

void setFilePathName(char *fileName) {
    strcpy(gPathName, fileName);
}

void sendCommand(char *command)
{
    int local_capFinish = 0;
    if(strncmp(command, "start", 5) == 0) {
        //wait last record compelement
        RECORD_LOCK()
        local_capFinish = capFinish;
        capturing = 1;
        RECORD_UNLOCK()

        while(local_capFinish != 1) {
            printf("last record not finish, wait a moment\n");
            RECORD_LOCK()
            local_capFinish = capFinish;
            RECORD_UNLOCK()
        }

        RECORD_LOCK()
        capFinish = 0;
        RECORD_UNLOCK()
        createCapturingThread();

    } else if(strncmp(command, "stop", 4) == 0) {
        RECORD_LOCK()
        capturing = 0;
        RECORD_UNLOCK()
    } else {
        printf("not support command: %s\n", command);
    }
}

int init_params(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-D card] [-d device] [-c channels] "
                "[-r rate] [-b bits] [-p period_size] [-n n_periods]\n", argv[0]);
        return 1;
    }

    /* parse command line arguments */
    argv += 1;
    while (*argv) {
        if (strcmp(*argv, "-d") == 0) {
            argv++;
            if (*argv)
                device = atoi(*argv);
        } else if (strcmp(*argv, "-c") == 0) {
            argv++;
            if (*argv)
                channels = atoi(*argv);
        } else if (strcmp(*argv, "-r") == 0) {
            argv++;
            if (*argv)
                rate = atoi(*argv);
        } else if (strcmp(*argv, "-b") == 0) {
            argv++;
            if (*argv)
                bits = atoi(*argv);
        } else if (strcmp(*argv, "-D") == 0) {
            argv++;
            if (*argv)
                card = atoi(*argv);
        } else if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                period_size = atoi(*argv);
        } else if (strcmp(*argv, "-n") == 0) {
            argv++;
            if (*argv)
                period_count = atoi(*argv);
        }
        if (*argv)
            argv++;
    }

    header.riff_id = ID_RIFF;
    header.riff_sz = 0;
    header.riff_fmt = ID_WAVE;
    header.fmt_id = ID_FMT;
    header.fmt_sz = 16;
    header.audio_format = FORMAT_PCM;
    header.num_channels = channels;
    header.sample_rate = rate;

    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        fprintf(stderr, "%d bits is not supported.\n", bits);
        return 1;
    }

    header.bits_per_sample = pcm_format_to_bits(format);
    header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
    header.block_align = channels * (header.bits_per_sample / 8);
    header.data_id = ID_DATA;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_mutex, &attr);

    return 0;
}

