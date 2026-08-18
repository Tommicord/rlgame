package net.rlgame;

import android.app.NativeActivity;
import android.os.Bundle;
import android.view.WindowManager;

public class Main extends NativeActivity
{
        static
        {
                System.loadLibrary ("rlgame_lib");
        }

        @Override
        protected void onCreate (Bundle savedInstanceState)
        {
                super.onCreate (savedInstanceState);
                
                getWindow ().addFlags (WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                getWindow ().addFlags (WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }

        public native void nativeOnCreate ();
        public native void nativeOnResume ();
        public native void nativeOnPause ();
        public native void nativeOnDestroy ();
        public native void nativeOnSurfaceCreated ();
        public native void nativeOnSurfaceChanged (int width, int height);
        public native void nativeOnSurfaceDestroyed ();
}
