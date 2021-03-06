/******************************************************************************/
/* writer-inplace.c  -- Writer that doesn't actually write pcap instead using
 *                      location of reading
 *
 * Copyright 2012-2017 AOL Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this Software except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _FILE_OFFSET_BITS 64
#include "moloch.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>

extern MolochConfig_t        config;


LOCAL MOLOCH_LOCK_DEFINE(filePtr2Id);

char                       *readerFileName[256];
LOCAL uint32_t              outputIds[256];

/******************************************************************************/
LOCAL uint32_t writer_inplace_queue_length()
{
    return 0;
}
/******************************************************************************/
LOCAL void writer_inplace_exit()
{
}
/******************************************************************************/
LOCAL long writer_inplace_create(MolochPacket_t * const packet)
{
    struct stat st;
    const char *readerName = readerFileName[packet->readerPos];

    stat(readerName, &st);

    uint32_t outputId;
    if (config.pcapReprocess) {
        moloch_db_file_exists(readerName, &outputId);
    } else {
        char *filename = moloch_db_create_file(packet->ts.tv_sec, readerName, st.st_size, 1, &outputId);
        g_free(filename);
    }
    outputIds[packet->readerPos] = outputId;
    return outputId;
}

/******************************************************************************/
LOCAL void writer_inplace_write(const MolochSession_t * const UNUSED(session), MolochPacket_t * const packet)
{
    MOLOCH_LOCK(filePtr2Id);
    long outputId = outputIds[packet->readerPos];
    if (!outputId)
        outputId = writer_inplace_create(packet);
    MOLOCH_UNLOCK(filePtr2Id);

    packet->writerFileNum = outputId;
    packet->writerFilePos = packet->readerFilePos;
}
/******************************************************************************/
LOCAL void writer_inplace_write_dryrun(const MolochSession_t * const UNUSED(session), MolochPacket_t * const packet)
{
    packet->writerFilePos = packet->readerFilePos;
}
/******************************************************************************/
void writer_inplace_init(char *UNUSED(name))
{
    moloch_writer_queue_length = writer_inplace_queue_length;
    moloch_writer_exit         = writer_inplace_exit;
    if (config.dryRun)
        moloch_writer_write    = writer_inplace_write_dryrun;
    else
        moloch_writer_write    = writer_inplace_write;
}
