#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTAsync.h>

#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "Publicaiton_Subscription"
#define TOPIC "xitong"
#define PAYLOAD "Hello"
#define QOS 1
#define TIMEOUT 10000L

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

void connlost(void *context, char *cause)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);

    printf("Reconnecting\n");
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start connect, return code %d\n", rc);
        finished = 1;
    }
}

int msgarrvd(void *contest, char *topicName, int topicLen, MQTTAsync_message * message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; ++i)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void deliveryCompleted()
{
    printf("delivery completed!\n");
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
        printf("Subscribe succeeded\n");
        subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Subscribe failed, rc %d\n", response ? response->code : 0);
        finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
        printf("Successful disconnection\n");
        finished = 1;
}

void onSend(void* context, MQTTAsync_successData* response)
{
    printf("Message sent. token value %d\n", response->token);
}

/** once connected, we can sub/pub messages. */
void onConnect(void *context, MQTTAsync_successData *response)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions sub_opts = MQTTAsync_responseOptions_initializer;

    int rc;

    printf("Connected!\n");

    /** we first subscribe */
    sub_opts.onSuccess = onSubscribe;
    sub_opts.onFailure = onSubscribeFailure;
    sub_opts.context = client;
    if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &sub_opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }

    /** then publish */
    MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    pub_opts.onSuccess = onSend;
    pub_opts.context = client;

    pubmsg.payload = PAYLOAD;
    pubmsg.payloadlen = strlen(PAYLOAD);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &pub_opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start sendMessage, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }

}

void onConnectFailure(void *context, MQTTAsync_failureData* response)
{

}

int main(int argc, char* argv[])
{
    /** First create a client object. */
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    int rc;
    int ch;

    /**if set to NULL, the persistence directory used is the working directory. */
    MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, deliveryCompleted);

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }

//    while	(!subscribed)
//            #if defined(WIN32) || defined(WIN64)
//                    Sleep(100);
//            #else
//                    usleep(10000L);
//            #endif

//    while (!finished)
//        usleep(10000L);

    do
    {
            ch = getchar();
    } while (ch!='Q' && ch != 'q');


    disc_opts.onSuccess = onDisconnect;
    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start disconnect, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }

    MQTTAsync_destroy(&client);
    return rc;

}
