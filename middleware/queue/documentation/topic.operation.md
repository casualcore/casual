# topic

**topic** or **publish-subscribe** is a messaging pattern where messages are categorized into topics. Publishers send messages to specific topics, and subscribers receive messages from the topics they are interested in.

`casual` does not directly provide a topic mechanism, but it can be quite easily built using queues, fanout and forwards.


## setup

To create a topic mechanism in `casual`, you can use the following components:
* **queues**: Create a queue for each topic. The queue name will represent the topic.
* **fanout**: Use fanout to distribute a topic message to multiple intermediary subscriber queues.
* **forwards**: Set up forwards to move messages from the intermediary subscriber queues to the final subscriber queues or services.


