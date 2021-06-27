//
// Created by season on 2021/6/27.
//

package com.huawei.zip;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * A helper class for unzip file
 */
public class ZipUtil {
    private static final int BUF_SIZE = 16 * 1024;
    public static final int FILESYSTEM_FILENAME_MAX_LENGTH = 255;


    public static boolean extractFileFromZip(ZipFile zipFile,
                                             String filePath,
                                             String dstFilePath) throws IOException {
        if (zipFile == null || filePath == null || dstFilePath == null) {
            return false;
        }
        if (dstFilePath.length() > FILESYSTEM_FILENAME_MAX_LENGTH) {
            return false;
        }
        ZipEntry entry = zipFile.getEntry(filePath);
        if (entry == null) {
            return false;
        }

        return writeFile(zipFile.getInputStream(entry), dstFilePath);
    }

    private static boolean writeFile(
            InputStream input, String dstFilePath) {
        try (InputStream in = new BufferedInputStream(input);
             OutputStream out = new BufferedOutputStream(new FileOutputStream(dstFilePath, false))) {
            byte[] buf = new byte[BUF_SIZE];
            int size;
            while ((size = in.read(buf)) != -1) {
                out.write(buf, 0, size);
            }
            out.flush();
            return true;
        } catch (IOException e) {
            //ignore
        }
        return false;
    }
}
