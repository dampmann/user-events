// g++ -Wall -W -o user_events user_events.cpp -I/usr/local/include -L/usr/local/lib -levent -lpthread -levent_pthreads
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

volatile bool stopped = false;

void user_event_cb(evutil_socket_t, short events, void *user_data) {
  std::cout << "User event fired!" << std::endl;
}

void signal_cb(evutil_socket_t sig, short events, void *user_data) {
  struct event_base *base = (struct event_base*)user_data;
  std::cerr << "Caught signal\n";
  event_base_loopexit(base, NULL);
}

void user_event_proc(struct event *ev) {
  std::cerr << "thread up!\n";
  while(!stopped) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    event_active(ev, EV_READ|EV_WRITE, 1);
  }
}

int main(int argc, char **argv) {
  struct event_base *base;
  struct event *signal_event;
  struct event *user_event;

  base = event_base_new();
  evthread_use_pthreads();

  if(evthread_make_base_notifiable(base) < 0) {
    std::cerr << "Couldn't make base notifiable!\n";
    return(1);
  }

  signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
  if(!signal_event || event_add(signal_event, NULL) < 0) {
    std::cerr << "Unable to create or add signal event\n";
    return(1);
  }

  user_event = event_new(base, -1, EV_PERSIST|EV_READ, user_event_cb, base);
  std::thread t { user_event_proc, user_event};
  event_base_dispatch(base);
  stopped = true;
  t.join();
  event_free(user_event);
  event_free(signal_event);
  event_base_free(base);
  return(0);
}



