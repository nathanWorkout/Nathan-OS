double NAN() {
    return 0.0 / 0.0;
}

double sqrt(double x) {
    double y = x; 

    if (x < 0.0) return NAN(); 

    if (x == 0.0) return 0.0;

    for (int i = 0; i < 10; i++) {
        double y_old = y;
        y = (y + x / y) / 2;
        double difference = y_old - y;
        if (difference < 0) difference = -difference;
        if (difference < 0.000001) break;
    }


    return y;
}