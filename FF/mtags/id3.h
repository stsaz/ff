/** ID3v1, ID3v2 tags.
Copyright (c) 2013 Simon Zolin
*/

#pragma once

#include <FF/mtags/mmtag.h>
#include <FF/array.h>


//128 bytes
typedef struct ffid31 {
	char tag[3]; //"TAG"
	char title[30];
	char artist[30];
	char album[30];
	char year[4];

	union {
	char comment30[30];

	// if comment30[28] == '\0':
	struct {
	char comment[29];
	byte track_no; //undefined: 0
	};};

	byte genre; //undefined: 0xff
} ffid31;

/** Return TRUE if valid ID3v1 header. */
static FFINL ffbool ffid31_valid(const ffid31 *h)
{
	return h->tag[0] == 'T' && h->tag[1] == 'A' && h->tag[2] == 'G';
}

typedef struct ffid31ex {
	uint state;
	char trkno[sizeof("255")];
	int field; //enum FFID3_FRAME
	ffstr val;
} ffid31ex;

enum FFID3_R {
	FFID3_RDONE
	, FFID3_RERR
	, FFID3_RDATA //ffid3.data contains a chunk of frame data
	, FFID3_RNO
};

/** Get the next value from ID3v1 tag.
Return enum FFID3_R. */
static inline int ffid31_parse(ffid31ex *id31ex, const char *data, size_t len)
{
	enum { I_HDR, I_TITLE, I_ARTIST, I_ALBUM, I_YEAR, I_COMMENT, I_TRK, I_GENRE, I_DONE };
	ffid31 *id31 = (ffid31*)data;
	uint n, *state = &id31ex->state;
	int r = FFID3_RDATA;
	ffstr *val = &id31ex->val;

	static const char *const id3_genres[] = {
		"Blues", "Classic Rock", "Country", "Dance", "Disco",
		"Funk", "Grunge", "Hip-Hop", "Jazz", "Metal",
		"New Age", "Oldies", "Other", "Pop", "R&B",
		"Rap", "Reggae", "Rock", "Techno", "Industrial",
		"Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
		"Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
		"Fusion", "Trance", "Classical", "Instrumental", "Acid",
		"House", "Game", "Sound Clip", "Gospel", "Noise",
		"AlternRock", "Bass", "Soul", "Punk", "Space",
		"Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic",
		"Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance",
		"Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
		"Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American",
		"Cabaret", "New Wave", "Psychadelic", "Rave", "Showtunes",
		"Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz",
		"Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock", //75-79
	};

	switch (*state) {

	case I_HDR:
		if (len != sizeof(ffid31) || !ffid31_valid(id31)) {
			r = FFID3_RNO;
			break;
		}
		*state = I_TITLE;
		// break

	case I_TITLE:
		if (id31->title[0] != '\0') {
			ffstr_setnz(val, id31->title, sizeof(id31->title));
			ffstr_rskip(val, ' ');
			if (val->len != 0) {
				id31ex->field = FFMMTAG_TITLE;
				*state = I_ARTIST;
				break;
			}
		}
		//break

	case I_ARTIST:
		if (id31->artist[0] != '\0') {
			ffstr_setnz(val, id31->artist, sizeof(id31->artist));
			ffstr_rskip(val, ' ');
			if (val->len != 0) {
				id31ex->field = FFMMTAG_ARTIST;
				*state = I_ALBUM;
				break;
			}
		}
		//break

	case I_ALBUM:
		if (id31->album[0] != '\0') {
			ffstr_setnz(val, id31->album, sizeof(id31->album));
			ffstr_rskip(val, ' ');
			if (val->len != 0) {
				id31ex->field = FFMMTAG_ALBUM;
				*state = I_YEAR;
				break;
			}
		}
		//break

	case I_YEAR:
		if (id31->year[0] != '\0') {
			ffstr_setnz(val, id31->year, sizeof(id31->year));
			ffstr_rskip(val, ' ');
			if (val->len != 0) {
				id31ex->field = FFMMTAG_DATE;
				*state = I_COMMENT;
				break;
			}
		}
		//break

	case I_COMMENT:
		if (id31->comment[0] != '\0') {
			n = (id31->comment30[28] != '\0') ? sizeof(id31->comment30) : sizeof(id31->comment);
			ffstr_setnz(val, id31->comment, n);
			ffstr_rskip(val, ' ');
			if (val->len != 0) {
				id31ex->field = FFMMTAG_COMMENT;
				*state = (id31->comment30[28] != '\0') ? I_DONE : I_TRK;
				break;
			}
		}
		//break

	case I_TRK:
		if (id31->track_no != 0) {
			n = ffs_fromint(id31->track_no, id31ex->trkno, sizeof(id31ex->trkno), FFINT_ZEROWIDTH | FFINT_WIDTH(2));
			ffstr_set(val, id31ex->trkno, n);
			id31ex->field = FFMMTAG_TRACKNO;
			*state = I_GENRE;
			break;
		}
		//break

	case I_GENRE:
		if (id31->genre < FFCNT(id3_genres)) {
			ffstr_setz(val, id3_genres[id31->genre]);
			id31ex->field = FFMMTAG_GENRE;
			*state = I_DONE;
			break;
		}
		//break

	case I_DONE:
		r = FFID3_RDONE;
		break;
	}

	return r;
}
