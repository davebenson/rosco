
RoscoSubscription * rosco_subscription_new         (RoscoNode            *node,
                                                    const char           *topic,
                                                    RoscoMessageType     *type);
void                rosco_subscription_destroy     (RoscoSubscription    *sub);


RoscoSubscriptionHandler *
                    rosco_subscription_add_handler (RoscoSubscription *subscription,
                                                    RoscoMessageHandler   handler,
                                                    void                 *handler_data,
                                                    RoscoDestroyFunc      handler_data_destroy);

void                rosco_subscription_handler_destroy (RoscoSubscriptionHandler *handler);
