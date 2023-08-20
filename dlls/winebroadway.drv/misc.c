/***********************************************************************
 *              ThreadDetach   (MACDRV.@)
 */
void broadwaydrv_ThreadDetach(void)
{
    struct macdrv_thread_data *data = macdrv_thread_data();

    if (data)
    {
        free(data);
        /* clear data in case we get re-entered from user32 before the thread is truly dead */
        NtUserGetThreadInfo()->driver_data = 0;
    }
}

