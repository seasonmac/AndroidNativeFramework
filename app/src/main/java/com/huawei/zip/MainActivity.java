package com.huawei.zip;

import androidx.appcompat.app.AppCompatActivity;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.ZipFile;

public class MainActivity extends AppCompatActivity {
    private static long gFileSize = 0;
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText("点击按钮进行性能测试");
        Button unzipTestButton = findViewById(R.id.button);
        String zipPath = getPackageResourcePath();
        String targetDir = getCacheDir().getAbsolutePath();
        unzipTestButton.setOnClickListener((view) -> {
            long start = System.currentTimeMillis();
            unzip(zipPath, "assets/manager.apk", targetDir);
            long native_time = System.currentTimeMillis() - start;
            Log.i("Perf_unzip_native", "cost time :" + native_time + "ms");

            start = System.currentTimeMillis();
            copyApk(getBaseContext(), "manager.apk", "manager_AssetManager.apk");
            long am_time = System.currentTimeMillis() - start;
            Log.i("Perf_unzip_assetmanager", "cost time :" + am_time + "ms");

            start = System.currentTimeMillis();
            copyApk2(zipPath, "assets/manager.apk", targetDir + "/manager_ZipFile.apk");
            long zipfile_time = System.currentTimeMillis() - start;
            Log.i("Perf_unzip_zipfile", "cost time :" + zipfile_time + "ms");

            try {
                gFileSize = new FileInputStream(targetDir + "/manager.apk").available()/1024;
            } catch (IOException e) {
                e.printStackTrace();
            }

            tv.setText(
                    "assets/Manager.apk文件大小：" + (gFileSize) + "KB \n" +
                    "Native解压耗时：" + native_time + " ms " + "\n" +
                    "AssetManager解压耗时：" + am_time + " ms " + "\n" +
                    "ZipFile解压耗时：" + zipfile_time + " ms " + "\n");

        });
    }

    private static boolean copyApk2(String zipPath, String extractFileName, String dstFilePath) {
        try (ZipFile zip = new ZipFile(zipPath)) {
            ZipUtil.extractFileFromZip(zip, extractFileName, dstFilePath);
        } catch (IOException ioe) {
        }
        return true;
    }

    private static boolean copyApk(Context ctx, String apkName, String targetApkName) {
        String tmpTargetApkName = targetApkName + ".tmp";
        String path = ctx.getCacheDir() + File.separator;
        File tmpTargetFile = new File(path, tmpTargetApkName);
        try (FileOutputStream outputStream = new FileOutputStream(tmpTargetFile);
             InputStream inputStream = ctx.getAssets().open(apkName);
             BufferedInputStream bufferedInputStream = new BufferedInputStream(inputStream)) {
            int count;
            FileDescriptor fd = null;
            byte[] buffer = new byte[4096];
            while ((count = bufferedInputStream.read(buffer, 0, buffer.length)) != -1) {
                outputStream.write(buffer, 0, count);
                outputStream.flush();
                if (fd == null) {
                    fd = outputStream.getFD();
                }
                fd.sync();
            }

            boolean succeeded = tmpTargetFile.renameTo(new File(path, targetApkName));
            if (!succeeded) {
                Log.w("Java_unzip", "copy " + apkName + " apk from asset failed");
                return false;
            }
            Log.i("Java_unzip", "copy " + apkName + " apk from asset successfully");
            return true;
        } catch (IOException e) {
            Log.e("Java_unzip", "Cannot get apkFile!", e);
        }
        return false;
    }

    public native boolean unzip(String zipPath, String fileName, String targetDir);
}