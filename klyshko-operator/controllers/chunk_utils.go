/*
Copyright (c) 2022-2026 - for information on the respective copyright owner
see the NOTICE file and/or the repository https://github.com/carbynestack/klyshko.

SPDX-License-Identifier: Apache-2.0
*/

package controllers

import (
	"fmt"

	"github.com/google/uuid"
)

// NumberOfChunks returns the number of chunks needed to cover count tuples when
// each chunk holds at most maxPerChunk tuples. Must match the provisioner's
// chunk count logic so that chunk IDs derived here agree with those produced
// by the provisioner.
func NumberOfChunks(count, maxPerChunk int) int {
	if maxPerChunk <= 0 {
		return 0
	}
	n := count / maxPerChunk
	if count%maxPerChunk != 0 {
		n++
	}
	return n
}

// DeriveChunkID returns a deterministic UUID v5 for the chunk identified by
// jobID and piece index. Uses the RFC 4122 URL namespace and name "<jobID>:<piece>"
// so that the result matches the provisioner's derive_chunk_id() exactly.
func DeriveChunkID(jobID uuid.UUID, piece, numChunks int) uuid.UUID {
	name := fmt.Sprintf("%s:%d", jobID.String(), piece)
	return uuid.NewSHA1(uuid.NameSpaceURL, []byte(name))
}
