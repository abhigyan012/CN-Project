#include <stdlib.h>
#include "../include/channel.h"

int should_drop(double probability) {

    double random_value =
        (double)rand() / RAND_MAX;

    return random_value < probability;
}

int should_corrupt(double probability) {

    double random_value =
        (double)rand() / RAND_MAX;

    return random_value < probability;
}
