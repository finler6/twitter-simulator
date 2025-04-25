#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TOTAL_USERS 100         // Total number of users
#define INFLUENTIAL_USERS 5     // Number of influential users
#define SIMULATION_TIME 1440    // Simulation time in minutes (1 day)
#define MAX_EVENTS 200000        // Maximum number of events
#define MAX_TWEETS 200000        // Maximum number of tweets
#define AVG_FOLLOWERS_ORDINARY 5   // Average number of followers for ordinary users
#define INFLUENTIAL_MULTIPLIER 6   // Influential users have 5 times more followers
#define AVG_FOLLOWERS_INFLUENTIAL (AVG_FOLLOWERS_ORDINARY * INFLUENTIAL_MULTIPLIER)
#define ACTIVE_DAY_START 8*60     // Start of active day (8:00)
#define ACTIVE_DAY_END 22*60      // End of active day (22:00)
#define PROBABILITY_FOLLOWER_RETWEET 33 // Probability of a follower retweeting a tweet

typedef enum { ORDINARY, INFLUENTIAL } UserType;
typedef enum { INACTIVE, ACTIVE } UserState;

typedef struct User {
    int id;
    UserType type;
    UserState state;
    int followersCount;
    int *followers;
    int followerCapacity;
} User;

typedef struct Tweet {
    int id;
    int userId;
    int isInfluential;
    int likes;
    int replies;
    int retweets;
    int timestamp;
} Tweet;

typedef enum { EVENT_ACTIVATE_USER, EVENT_USER_ACTION } EventType;

typedef struct Event {
    int time;           // Event time
    EventType type;     // Event type
    int userId;         // User ID
} Event;

User users[TOTAL_USERS];
Tweet tweets[MAX_TWEETS];
int tweetCount = 0;

Event eventQueue[MAX_EVENTS];
int eventCount = 0;

int currentTime = 0;

void addFollower(int userId, int followerId);
void initializeUsers();
void scheduleEvent(int time, EventType type, int userId);
int compareEvents(const void *a, const void *b);
void activateUsers();
void processEvent(Event event);
void composeTweet(int userId, int timestamp);
void retweet(int userId, int timestamp);
void likeTweet(int timestamp);
void replyTweet(int userId, int timestamp);
void runSimulation();
void outputStatistics();
void freeMemory();
// Statistics
int totalLikes = 0, totalReplies = 0, totalRetweets = 0;
int actionsPerTime[SIMULATION_TIME];

void addFollower(int userId, int followerId) {
    if (followerId < 0 || followerId >= TOTAL_USERS) return; // Check validity of followerId
    User *user = &users[userId];
    if (user->followersCount == user->followerCapacity) {
        user->followerCapacity *= 2;
        user->followers = realloc(user->followers, user->followerCapacity * sizeof(int));
    }
    user->followers[user->followersCount++] = followerId;
}


void initializeUsers() {
    for (int i = 0; i < TOTAL_USERS; i++) {
        users[i].id = i;
        users[i].state = INACTIVE;
        users[i].followersCount = 0;
        users[i].followerCapacity = 10;
        users[i].followers = malloc(users[i].followerCapacity * sizeof(int));
        if (i < INFLUENTIAL_USERS) {
            users[i].type = INFLUENTIAL;
        } else {
            users[i].type = ORDINARY;
        }
    }

    // Create followers
    for (int i = 0; i < TOTAL_USERS; i++) {
        int followersNum = (users[i].type == INFLUENTIAL) ? 
                        (rand() % AVG_FOLLOWERS_INFLUENTIAL) : 
                        (rand() % AVG_FOLLOWERS_ORDINARY + AVG_FOLLOWERS_ORDINARY / 5);
        if (followersNum > TOTAL_USERS - 1) followersNum = TOTAL_USERS - 1;
        for (int j = 0; j < followersNum; j++) {
            int followerId = rand() % TOTAL_USERS;
            if (followerId != i) {
                addFollower(i, followerId);
            }
        }
    }
}

void scheduleEvent(int time, EventType type, int userId) {
    if (eventCount >= MAX_EVENTS) return;
    if (userId < 0 || userId >= TOTAL_USERS) return; // Check userId
    if (time < 0 || time >= SIMULATION_TIME) return; // Check time
    Event newEvent;
    newEvent.time = time;
    newEvent.type = type;
    newEvent.userId = userId;
    eventQueue[eventCount++] = newEvent;
}


// Comparison function for qsort
int compareEvents(const void *a, const void *b) {
    Event *eventA = (Event *)a;
    Event *eventB = (Event *)b;
    return eventA->time - eventB->time;
}

void activateUsers() {
    for (int i = 0; i < TOTAL_USERS; i++) {
        // Activation probability depends on the time of day
        int activationChance = (currentTime >= ACTIVE_DAY_START && currentTime <= ACTIVE_DAY_END) ? 30 : 10; 
        if (users[i].state == INACTIVE && (rand() % 100) < activationChance) {
            int delay = rand() % 10; // Delay before activation
            scheduleEvent(currentTime + delay, EVENT_ACTIVATE_USER, i);
        }
    }
}

void processEvent(Event event) {
    User *user = &users[event.userId];
    if (event.type == EVENT_ACTIVATE_USER) {
        user->state = ACTIVE;
        int actionDelay = rand() % 5 + 1; // Delay before action
        scheduleEvent(event.time + actionDelay, EVENT_USER_ACTION, event.userId);
    } else if (event.type == EVENT_USER_ACTION) {
        if (user->state == ACTIVE) {
            int actionProb = rand() % 100;
            if (actionProb < 40) {
                composeTweet(event.userId, event.time);
            } else if (actionProb < 70) {
                retweet(event.userId, event.time);
            } else if (actionProb < 85) {
                likeTweet(event.time);
            } else {
                replyTweet(event.userId, event.time);
            }
            user->state = INACTIVE;
        }
    }
}

void composeTweet(int userId, int timestamp) {
    if (userId < 0 || userId >= TOTAL_USERS) return;
    if (tweetCount >= MAX_TWEETS) return;
    Tweet newTweet;
    newTweet.id = tweetCount;
    newTweet.userId = userId;
    newTweet.isInfluential = (users[userId].type == INFLUENTIAL);
    newTweet.likes = 0;
    newTweet.replies = 0;
    newTweet.retweets = 0;
    newTweet.timestamp = timestamp;
    tweets[tweetCount++] = newTweet;

    // Spread the tweet to followers
    User *user = &users[userId];
    for (int i = 0; i < user->followersCount; i++) {
        int followerId = user->followers[i];
        if (followerId < 0 || followerId >= TOTAL_USERS) continue;
        if ((rand() % 100) < PROBABILITY_FOLLOWER_RETWEET) {
            int delay = rand() % 10;
            scheduleEvent(timestamp + delay, EVENT_ACTIVATE_USER, followerId);
        }
    }

    if (timestamp >= 0 && timestamp < SIMULATION_TIME) {
        actionsPerTime[timestamp]++;
    }
}

void retweet(int userId, int timestamp) {
    if (userId < 0 || userId >= TOTAL_USERS) return; 
    if (tweetCount == 0) return;
    int tweetId = rand() % tweetCount;
    Tweet *tweet = &tweets[tweetId];
    tweet->retweets++;
    totalRetweets++;
    User *user = &users[userId];
    for (int i = 0; i < user->followersCount; i++) {
        int followerId = user->followers[i];
        if (followerId < 0 || followerId >= TOTAL_USERS) continue; 
        if ((rand() % 100) < PROBABILITY_FOLLOWER_RETWEET) { 
            int delay = rand() % 10;
            scheduleEvent(timestamp + delay, EVENT_ACTIVATE_USER, followerId);
        }
    }

    if (timestamp >= 0 && timestamp < SIMULATION_TIME) {
        actionsPerTime[timestamp]++;
    }
}

void likeTweet(int timestamp) {
    if (tweetCount == 0) return;
    int tweetId = rand() % tweetCount;
    tweets[tweetId].likes++;
    totalLikes++;
    actionsPerTime[timestamp]++;
}

void replyTweet(int userId, int timestamp) {
    if (userId < 0 || userId >= TOTAL_USERS) return;
    if (tweetCount == 0) return;
    int tweetId = rand() % tweetCount;
    tweets[tweetId].replies++;
    totalReplies++;

    User *user = &users[userId];
    for (int i = 0; i < user->followersCount; i++) {
        int followerId = user->followers[i];
        if (followerId < 0 || followerId >= TOTAL_USERS) continue;
        if ((rand() % 100) < PROBABILITY_FOLLOWER_RETWEET) { 
            int delay = rand() % 10;
            scheduleEvent(timestamp + delay, EVENT_ACTIVATE_USER, followerId);
        }
    }

    if (timestamp >= 0 && timestamp < SIMULATION_TIME) {
        actionsPerTime[timestamp]++;
    }
}


void runSimulation() {
    while ((currentTime < SIMULATION_TIME) || (eventCount > 0)) {
        // Activate users at the beginning of each minute interval
        if (currentTime < SIMULATION_TIME) {
            activateUsers();
        }

        // Process events scheduled for the current moment
        for (int i = 0; i < eventCount; ) {
            if (eventQueue[i].time <= currentTime) {
                processEvent(eventQueue[i]);
                // Remove the event from the queue
                for (int j = i; j < eventCount - 1; j++) {
                    eventQueue[j] = eventQueue[j + 1];
                }
                eventCount--;
            } else {
                i++;
            }
        }
        if (currentTime >= SIMULATION_TIME) {
            currentTime = SIMULATION_TIME; // Prevent currentTime from increasing
        } else {
            currentTime++;
        }
    }
}


void outputStatistics() {
    int influentialTweets = 0;
    int ordinaryTweets = 0;
    for (int i = 0; i < tweetCount; i++) {
        if (tweets[i].isInfluential)
            influentialTweets++;
        else
            ordinaryTweets++;
    }

    printf("Simulace dokončena.\n");
    printf("Celkový počet tweetů: %d\n", tweetCount);
    printf(" - Tweety od vlivných uživatelů: %d\n", influentialTweets);
    printf(" - Tweety od běžných uživatelů: %d\n", ordinaryTweets);
    printf("Celkový počet retweetů: %d\n", totalRetweets);
    printf("Celkový počet lajků: %d\n", totalLikes);
    printf("Celkový počet odpovědí: %d\n", totalReplies);

    double avgLikes = tweetCount > 0 ? (double)totalLikes / tweetCount : 0;
    double avgReplies = tweetCount > 0 ? (double)totalReplies / tweetCount : 0;
    double avgRetweets = tweetCount > 0 ? (double)totalRetweets / tweetCount : 0;

    printf("Průměrný počet lajků na tweet: %.2f\n", avgLikes);
    printf("Průměrný počet odpovědí na tweet: %.2f\n", avgReplies);
    printf("Průměrný počet retweetů na tweet: %.2f\n", avgRetweets);

    int actionsPerHour[24] = {0};
    for (int i = 0; i < SIMULATION_TIME; i++) {
        int hour = i / 60;
        if (hour < 24) {
            actionsPerHour[hour] += actionsPerTime[i];
        }
    }

    printf("\nPočet akcí za hodinu:\n");
    for (int i = 0; i < 24; i++) {
        printf("Hodina %02d:00 - %02d:59 : %d akcí\n", i, i, actionsPerHour[i]);
    }
}

void freeMemory() {
    for (int i = 0; i < TOTAL_USERS; i++) {
        free(users[i].followers);
    }
}

int main() {
    srand(time(NULL));
    initializeUsers();
    runSimulation();
    outputStatistics();
    freeMemory();
    return 0;
}
