package com.github.preferme.ipc;

public class Hex {

    public static final char[] HEXs = new char[] {
            '0', '1', '2', '3', '4', '5','6', '7',
            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    public static String toString(byte[] buffer, int offset, int lengh) {
        StringBuilder builder = new StringBuilder(lengh << 1);
        for (int i=offset; i<lengh; i++) {
            byte b = buffer[i];
            builder.append(HEXs[(buffer[i]>>4)]).append(HEXs[(buffer[i] & 0x0F)]);
        }
        return builder.toString();
    }

    public static String toString(byte[] buffer) {
        return toString(buffer, 0, buffer.length);
    }
}
