#include <gst/gst.h>

#define CNT 20
struct BUF {
    guint32 b[384 * 288];
} ab[CNT];


static GstPadProbeReturn
cb_have_data(GstPad* pad,
    GstPadProbeInfo* info,
    gpointer         user_data)
{
    gint x, y;
    GstMapInfo map;
    guint32* ptr, t;
    GstBuffer* buffer;
    static long cnt = 0;
    buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    buffer = gst_buffer_make_writable(buffer);

    /* Making a buffer writable can fail (for example if it
     * cannot be copied and is used more than once)
     */
    if (buffer == NULL)
        return GST_PAD_PROBE_OK;
    
    gst_print("in cb\n");
    /* Mapping a buffer can fail (non-writable) */
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        gst_print("cnt=%ld\n",cnt);
        if (cnt < CNT) {
            memcpy(ab[cnt].b, map.data, map.size);
            memset(map.data, cnt*5+100, map.size);
        }
        else {
            memcpy(ab[(cnt-1)%CNT].b, map.data, map.size);
            memcpy(map.data, ab[cnt%CNT].b, map.size);
        }

        //ptr = (guint32*)map.data;
        ///* invert data */
        //for (y = 0; y < 288; y++) {
        //    for (x = 0; x < 384 / 2; x++) {
        //        t = ptr[384 - 1 - x];
        //        ptr[384 - 1 - x] = ptr[x];
        //        ptr[x] = t;// +cnt;
        //    }
        //    ptr += 384;
        //}
        gst_buffer_unmap(buffer, &map);
    }

    GST_PAD_PROBE_INFO_DATA(info) = buffer;
    cnt++;
    if (cnt > CNT * 10)
        cnt = CNT + cnt % cnt;
    return GST_PAD_PROBE_OK;
}


int main(int argc, char* argv[])
{
    GstElement* pipeline, * inj;
    GstBus* bus;
    GstMessage* msg;
    GstPad* pad;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Build the pipeline */
    pipeline =
        gst_parse_launch
        //("videotestsrc pattern=ball  ! video/x-raw,width=384,height=288,framerate=(fraction)2/1 !  videoconvert ! clockoverlay ypad=88,name=inj ! clockoverlay ! ximagesink", NULL);
        ("videotestsrc pattern=ball  ! video/x-raw,width=384,height=288,framerate=(fraction)4/1 !  videoconvert ! clockoverlay name=inj ! clockoverlay ypad=88 ! ximagesink", NULL);
        //("videotestsrc pattern=ball  ! video/x-raw,width=360,height=140 ! videoconvert ! clockoverlay ! ximagesink", NULL);

     
    inj = gst_bin_get_by_name(GST_BIN(pipeline), "inj");

    pad = gst_element_get_static_pad(inj, "src");
        gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER,
            (GstPadProbeCallback)cb_have_data, NULL, NULL);
        gst_object_unref(pad);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    gst_print("Wait until error or EOS\n");
    /* Wait until error or EOS */ 
    bus = gst_element_get_bus(pipeline);
    
    msg =
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
            (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    
    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
