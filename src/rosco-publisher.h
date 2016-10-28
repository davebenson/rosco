
RoscoPublisher    * rosco_publisher_new            (RoscoNode            *node,
                                                    const char           *topic,
                                                    RoscoMessageType     *type);
void                rosco_publisher_destroy        (RoscoPublisher       *publisher);

void                rosco_publish                  (RoscoPublisher       *publisher,
                                                    RoscoMessage         *message);

