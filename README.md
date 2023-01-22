# Producer-Consumer-Problem
Using posix semaphores and shared memory to work on the producer consumer problem.

### Producers
Each producer is supposed to continuously declare the price of one commodity. For simplicity, we assumethat each commodity price follows a normal distribution. Therefore, producers will generate a new random price, share it with the consumer, and then sleep for an interval before producing the next price.

### Consumer
The consumer is to print the current price of each commodity, along the average of the current and past 4 readings. An Up/Down arrow to show whether the current Price (AvgPrice) got increased or decreased from the prior one. Until you receive current prices, use 0.00 for the current price of any commodity.

## User Manual
Open two seperate terminals one for producer and another for consumer
Type make in each of them

for the consumer type ./consumer [buffer_size]
ex:
./consumer 20

for the producer type ./producer [commodity_name] [price_mean] [price_standard_deviation] [sleep_interval_ms] [buffer_size]
ex:
./consumer 20
./producer ALUMINIUM 100 6 5000 20 &
./producer COPPER 50 30 500 20 &
./producer COTTON 20 35 1000 20 &
./producer CRUDEOIL 15000 14 500 20 &
./producer GOLD 1000 300 600 20 &
./producer LEAD 60 20 700 20 &
./producer MENTHAOIL 500 30 2500 20 &
./producer NATURALGAS 300 37 3500 20 &
./producer NICKEL 200 300 690 20 &
./producer SILVER 600 20 5700 20 &
./producer ZINC 36 42 510 20 &

### Results
[video][producer_consumer.mp4]
