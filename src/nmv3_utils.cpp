#include <stdint.h>

#ifdef ARDUINO // ARDUINO
#include <Arduino.h>

#ifdef min // min
#undef min
#endif //min

#ifdef max //min
#undef max
#endif //min

#endif // ARDUINO

// ----- Helper functions -----

int
fieldToInt(
    char* field,
    uint8_t field_len
){
    char temp[field_len + 1] = {0};

    memcpy(temp, field, field_len);
    temp[field_len] = '\0';

    return atoi(temp);
}

void
printBufferContents(
    uint8_t* buf,
    uint8_t size
){
    if (size == 0) {
        Serial.printf("\tOops! This buffer is empty!\r\n"); 
        return;
    }

    Serial.printf("\tBuffer Contents (%03u bytes): \r\n", size);
    Serial.printf("\t    |     0          1          2          3          4          5          6          7\r\n");

    int i = 0;
    for (; i < size - 1; i++) {
        if (i % 8 == 0)
            Serial.printf("\r\n\t%2i |", i / 8);

        Serial.printf("%c (%03u), ", (char)buf[i], buf[i]);
    }

    // Final byte — no trailing comma
    if (i % 8 == 0)
        Serial.printf("\r\n\t%2i |", i / 8);

    Serial.printf("%c (%03u)\r\n", (char)buf[i], buf[i]);
}