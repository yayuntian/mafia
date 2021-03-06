FLAGS = -Wall -g -O0 #-Werror
FLAGS += -DPERF
FLAGS += -DLRU_CACHE
CXXFLAGS += -std=c++0x $(FLAGS)
CFLAGS += -std=c99 -msse4.2 $(FLAGS)

LIBS = -lrdkafka -lboost_regex -levent -lpthread

OBJS = main.o kafkaConsumer.o userAgent.o \
            operatingSystem.o bot.o \
	        browser.o extractor.o enricher.o \
	        ipLocator.o wrapper.o http.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: mafia #json_test ua_test ip_test

mafia: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

# unit test

JSON_OBJS = test_json.o userAgent.o \
                operatingSystem.o bot.o \
                browser.o extractor.o enricher.o \
                ipLocator.o wrapper.o http.o
json_test: $(JSON_OBJS)
	$(CXX) -o $@ $(JSON_OBJS) $(LIBS)

UA_OBJS = test_ua.o userAgent.o \
                operatingSystem.o bot.o \
                browser.o extractor.o enricher.o \
                ipLocator.o wrapper.o http.o
ua_test: $(UA_OBJS)
	$(CXX) -o $@ $(UA_OBJS) $(LIBS)

IP_OBJS = test_ip.o userAgent.o \
                operatingSystem.o bot.o \
                browser.o extractor.o enricher.o \
                ipLocator.o wrapper.o http.o
ip_test: $(IP_OBJS)
	$(CXX) -o $@ $(IP_OBJS) $(LIBS)


.PHONY: clean
clean:
	rm -f *.o mafia json_test ua_test ip_test
