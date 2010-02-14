/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id: codec.h 26239 2007-05-10 01:53:20Z gmaynard $

 ********************************************************************/

/**\mainpage
 * 
 * \section intro Introduction
 *
 * This is the documentation for the <tt>libtheora</tt> C API.
 * <tt>libtheora</tt> is the reference implementation for
 *  <a href="http://www.theora.org/">Theora</a>, a free video codec.
 * Theora is derived from On2's VP3 codec with additional features and
 *  integration for Ogg multimedia formats by
 *  <a href="http://www.xiph.org/">the Xiph.Org Foundation</a>.
 * Complete documentation of the format itself is available in
 * <a href="http://www.theora.org/doc/Theora_I_spec.pdf">the Theora
 *  specification</a>.
 *
 * \subsection Organization
 *
 * <tt>libtheora</tt> is actually subdivided into three separate libraries:
 * - <tt>libtheoraenc</tt>, containing routines exclusive to the encoder.
 *   You must link to this if you use any of the functions listed in
 *    \ref encfuncs.
 * - <tt>libtheoradec</tt>, containing routines exclusive to the decoder.
 *   You must link to this if you use any of the functions listed in
 *    \ref decfuncs.
 * - <tt>libtheorabase</tt>, containing routines shared by the encoder and the
 *    decoder.
 *   You must link to this if you use any of the functions in this API, not
 *    not just those listed in \ref basefuncs.*/

/**\file
 * The shared <tt>libtheora</tt> C API.*/

#if !defined(_O_THEORA_CODEC_H_)
# define _O_THEORA_CODEC_H_ (1)
# include <ogg/ogg.h>

#if defined(__cplusplus)
extern "C" {
#endif



/**\name Error codes*/
/*@{*/
/**An invalid pointer was provided.*/
#define OC_FAULT     (-1)
/**An invalid argument was provided.*/
#define OC_EINVAL    (-10)
/**The contents of the header were incomplete, invalid, or unexpected.*/
#define OC_BADHEADER (-20)
/**The header does not belong to a Theora stream.*/
#define OC_NOTFORMAT (-21)
/**The bitstream version is too high.*/
#define OC_VERSION   (-22)
/**The specified function is not implemented.*/
#define OC_IMPL      (-23)
/**There were errors in the video data packet.*/
#define OC_BADPACKET (-24)
/*@}*/

/**The currently defined color space tags.
 * See <a href="http://www.theora.org/doc/Theora_I_spec.pdf">the Theora
 *  specification</a>, Chapter 4, for exact details on the meaning of each of
 *  these color spaces.*/
typedef enum{
  /**The color space was not specified at the encoder.
      It may be conveyed by an external means.*/
  OC_CS_UNSPECIFIED,
  /**A color space designed for NTSC content.*/
  OC_CS_ITU_REC_470M,
  /**A color space designed for PAL/SECAM content.*/
  OC_CS_ITU_REC_470BG,
  /**The total number of currently defined color spaces.*/
  OC_CS_NSPACES
}theora_colorspace;

/**The currently defined pixel format tags.
 * See <a href="http://www.theora.org/doc/Theora_I_spec.pdf">the Theora
 *  specification</a>, Section 4.4, for details on the precise sample
 *  locations.*/
typedef enum{
  /**Chroma decimation by 2 in both the X and Y directions (4:2:0).*/
  OC_PF_420,
  /**Currently reserved.*/
  OC_PF_RSVD,
  /**Chroma decimation by 2 in the X direction (4:2:2).*/
  OC_PF_422,
  /**No chroma decimation (4:4:4).*/
  OC_PF_444,
  /**The total number of currently defined pixel formats.*/
  OC_PF_NFORMATS
}theora_pixel_fmt;



/**A buffer for a single color plane in an uncompressed image.
 * This contains the image data in a left-to-right, top-down format.
 * Each row of pixels is stored contiguously in memory, but successive rows
 *  need not be.
 * Use \a ystride to compute the offset of the next row.
 * The encoder accepts both positive \a ystride values (top-down) and negative
 *  (bottom-up).
 * The decoder currently always generates images with positive strides.
 */
typedef struct{
  /**The width of this plane.*/
  int            width;
  /**The height of this plane.*/
  int            height;
  /**The offset in bytes between successive rows.*/
  int            ystride;
  /**A pointer to the first row.*/
  unsigned char *data;
}theora_img_plane;

/**A complete image buffer for an uncompressed frame.
 * The chroma planes may be decimated by a factor of two in either direction,
 *  as indicated by theora_info#pixel_fmt.
 * The width and height of the Y' plane must be multiples of 16.
 * They may need to be cropped for display, using the rectangle specified by
 *  theora_info#pic_x, theora_info#pic_y, theora_info#pic_width, and
 *  theora_info#pic_height.
 * All samples are 8 bits.
 * \note The term YUV often used to describe a colorspace is ambiguous.
 * The exact parameters of the RGB to YUV conversion process aside, in many
 *  contexts the U and V channels actually have opposite meanings.
 * To avoid this confusion, we are explicit: the name of the color channels is
 *  Y'CbCr, and they appear in that order, always.
 * The prime symbol denotes that the Y channel is non-linear.
 * Cb and Cr stand for "Chroma blue" and "Chroma red" respectively.*/
typedef theora_img_plane theora_ycbcr_buffer[3];

/**Theora bitstream information.
 * This contains the basic playback parameters for a stream, and corresponds to 
 *  the initial 'info' header packet.
 * To initialize an encoder, the application fills in this structure and
 *  passes it to theora_encode_alloc().
 * A default encoding mode is chosen based on the values of the #quality and
 *  #target_bitrate fields.
 * On decode, it is filled in by theora_decode_headerin(), and then passed to
 *  theora_decode_alloc().
 * 
 * Encoded Theora frames must be a multiple of 16 in size;
 *  this is what the #frame_width and #frame_height members represent.
 * To handle arbitrary picture sizes, a crop rectangle is specified in the
 *  #pic_x, #pic_y, #pic_width and #pic_height members.
 *
 * All frame buffers contain pointers to the full, padded frame.
 * However, the current encoder <em>will not</em> reference pixels outside of
 *  the cropped picture region, and the application does not need to fill them
 *  in.
 * The decoder <em>will</em> allocate storage for a full frame, but the
 *  application <em>should not</em> rely on the padding containing sensible
 *  data.
 *
 * It is also generally recommended that the offsets and sizes should still be
 *  multiples of 2 to avoid chroma sampling shifts.
 * See <a href="http://www.theora.org/doc/Theora_I_spec.pdf">the Theora
 *  specification</a>, Section 4.4, for more details.
 *
 * Frame rate, in frames per second, is stored as a rational fraction, as is
 *  the pixel aspect ratio.
 * Note that this refers to the aspect ratio of the individual pixels, not of
 *  the overall frame itself.
 * The frame aspect ratio can be computed from pixel aspect ratio using the
 *  image dimensions.
 */
typedef struct{
  /**\name Theora version
   * Bitstream version information.*/
  /*@{*/
  unsigned char      version_major;
  unsigned char      version_minor;
  unsigned char      version_subminor;
  /*@}*/
  /**The encoded frame width.
   * This must be a multiple of 16, and less than 1048576.*/
  ogg_uint32_t       frame_width;
  /**The encoded frame height.
   * This must be a multiple of 16, and less than 1048576.*/
  ogg_uint32_t       frame_height;
  /**The displayed picture width.
   * This must be no larger than width.*/
  ogg_uint32_t       pic_width;
  /**The displayed picture height.
   * This must be no larger than height.*/
  ogg_uint32_t       pic_height;
  /**The X offset of the displayed picture.
   * This must be no larger than #frame_width-#pic_width or 255, whichever is
   *  smaller.*/
  ogg_uint32_t       pic_x;
  /**The Y offset of the displayed picture.
   * This must be no larger than #frame_height-#pic_height, and
   *  #frame_height-#pic_height-#pic_y must be no larger than 255.
   * This offset is specified from the top of the image, as this API uses the
   *  standard graphics left-handed coordinate system.*/
  ogg_uint32_t       pic_y;
  /**\name Frame rate
   * The frame rate, as a fraction.
   * If either is 0, the frame rate is undefined.*/
  /*@{*/
  ogg_uint32_t       fps_numerator;
  ogg_uint32_t       fps_denominator;
  /*@}*/
  /**\name Aspect ratio
   * The aspect ratio of the pixels.
   * If either value is zero, the aspect ratio is undefined.
   * If not specified by any external means, 1:1 should be assumed.*/
  /*@{*/
  ogg_uint32_t       aspect_numerator;
  ogg_uint32_t       aspect_denominator;
  /*@}*/
  /**The color space.*/
  theora_colorspace  colorspace;
  /**The pixel format.*/
  theora_pixel_fmt   pixel_fmt;
  /**The target bit-rate in bits per second.
     If initializing an encoder with this struct, set this field to a non-zero
      value to activate CBR encoding by default.*/
  /*TODO: Current encoder does not support CBR mode, or anything like it.
    We also don't really know what nominal rate each quality level
     corresponds to yet.*/
  int                target_bitrate;
  /**The target quality level.
     Valid values range from 0 to 63, inclusive, with higher values giving
      higher quality.
     If initializing an encoder with this struct, and #target_bitrate is set
      to zero, VBR encoding at this quality will be activated by default.*/
  /*Currently this is set so that a qi of 0 corresponds to distortions of 24
     times the JND, and each increase by 16 halves that value.
    This gives us fine discrimination at low qualities, yet effective rate
     control at high qualities.
    The qi value 63 is special, however.
    For this, the highest quality, we use one half of a JND for our threshold.
    Due to the lower bounds placed on allowable quantizers in Theora, we will
     not actually be able to achieve quality this good, but this should
     provide as close to visually lossless quality as Theora is capable of.
    We could lift the quantizer restrictions without breaking VP3.1
     compatibility, but this would result in quantized coefficients that are
     too large for the current bitstream to be able to store.
    We'd have to redesign the token syntax to store these large coefficients,
     which would make transcoding complex.*/
  int                quality;
  /**The amount to shift to extract the last keyframe number from the granule
   *  position.
   * This can be at most 31.
   * theora_info_init() will set this to a default value (currently <tt>6</tt>,
   *  which is good for streaming applications), but you can set it to 0 to
   *  make every frame a keyframe.
   * The maximum distance between key frames is
   *  <tt>1<<#keyframe_granule_shift</tt>.
   * The keyframe frequency can be more finely controlled with
   *  #OC_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, which can also be adjusted
   *  during encoding (for example, to force the next frame to be a keyframe),
   *  but it cannot be set larger than the amount permitted by this field after
   *  the headers have been output.*/
  int                keyframe_granule_shift;
}theora_info;

/**The comment information.
 *
 * This structure holds the in-stream metadata corresponding to
 *  the 'comment' header packet.
 * The comment header is meant to be used much like someone jotting a quick
 *  note on the label of a video.
 * It should be a short, to the point text note that can be more than a couple
 *  words, but not more than a short paragraph.
 *
 * The metadata is stored as a series of (tag, value) pairs, in
 *  length-encoded string vectors.
 * The first occurrence of the '=' character delimits the tag and value.
 * A particular tag may occur more than once, and order is significant.
 * The character set encoding for the strings is always UTF-8, but the tag
 *  names are limited to ASCII, and treated as case-insensitive.
 * See <a href="http://www.theora.org/doc/Theora_I_spec.pdf">the Theora
 *  specification</a>, Section 6.3.3 for details.
 *
 * In filling in this structure, theora_decode_header() will null-terminate
 *  the user_comment strings for safety.
 * However, the bitstream format itself treats them as 8-bit clean vectors,
 *  possibly containing null characters, and so the length array should be
 *  treated as their authoritative length.
 */
typedef struct theora_comment{
  /**The array of comment string vectors.*/
  char **user_comments;
  /**An array of the corresponding length of each vector, in bytes.*/
  int   *comment_lengths;
  /**The total number of comment strings.*/
  int    comments;
  /**The null-terminated vendor string.
     This identifies the software used to encode the stream.*/
  char  *vendor;
}theora_comment;



/**A single base matrix.*/
typedef unsigned char theora_quant_base[64];

/**A set of \a qi ranges.*/
typedef struct{
  /**The number of ranges in the set.*/
  int                      nranges;
  /**The size of each of the #nranges ranges.
     These must sum to 63.*/
  const int               *sizes;
  /**#nranges <tt>+1</tt> base matrices.
     Matrices \a i and <tt>i+1</tt> form the endpoints of range \a i.*/
  const theora_quant_base *base_matrices;
}theora_quant_ranges;

/**A complete set of quantization parameters.
   The quantizer for each coefficient is calculated as:
   \code
    Q=MAX(MIN(qmin[qti][ci!=0],scale[ci!=0][qi]*base[qti][pli][qi][ci]/100),
     1024).
   \endcode

   \a qti is the quantization type index: 0 for intra, 1 for inter.
   <tt>ci!=0</tt> is 0 for the DC coefficient and 1 for the AC coefficient.
   \a qi is the quality index, ranging between 0 (low quality) and 63 (high
    quality).
   \a pli is the color plane index: 0 for Y', 1 for Cb, 2 for Cr.
   \a ci is the DCT coefficient index.
   Coefficient indices correspond to the normal 2D DCT block
    ordering--row-major with low frequencies first--\em not zig-zag order.

   Minimum quantizers are constant, and are given by:
   \code
   qmin[2][2]={{4,2},{8,4}}.
   \endcode

   Parameters that can be stored in the bitstream are as follows:
    - The two scale matrices ac_scale and dc_scale.
      \code 
      scale[2][64]={dc_scale,ac_scale}.
      \endcode
    - The base matrices for each \a qi, \a qti and \a pli (up to 384 in all).
      In order to avoid storing a full 384 base matrices, only a sparse set of
       matrices is stored, and the rest are linearly interpolated.
      This is done as follows.
      For each \a qti and \a pli, a series of \a n \a qi ranges is defined.
      The size of each \a qi range can vary arbitrarily, but they must sum to
       63.
      Then, <tt>n+1</tt> matrices are specified, one for each endpoint of the
       ranges.
      For interpolation purposes, each range's endpoints are the first \a qi
       value it contains and one past the last \a qi value it contains.
      Fractional values are rounded to the nearest integer, with ties rounded
       away from zero.

      Base matrices are stored by reference, so if the same matrices are used
       multiple times, they will only appear once in the bitstream.
      The bitstream is also capable of omitting an entire set of ranges and
       its associated matrices if they are the same as either the previous
       set (indexed in row-major order) or if the inter set is the same as the
       intra set.

    - Loop filter limit values.
      The same limits are used for the loop filter in all color planes, despite
       potentially differing levels of quantization in each.

   For the current encoder, <tt>scale[ci!=0][qi]</tt> must be no greater
    than <tt>scale[ci!=0][qi-1]</tt> and <tt>base[qti][pli][qi][ci]</tt> must
    be no greater than <tt>base[qti][pli][qi-1][ci]</tt>.
   These two conditions ensure that the actual quantizer for a given \a qti,
    \a pli, and \a ci does not increase as \a qi increases.
   This is not required by the decoder.*/
typedef struct{
  /**The DC scaling factors.*/
  ogg_uint16_t        dc_scale[64];
  /**The AC scaling factors.*/
  ogg_uint16_t        ac_scale[64];
  /**The loop filter limit values.*/
  unsigned char       loop_filter_limits[64];
  /**The \a qi ranges for each \a ci and \a pli.*/
  theora_quant_ranges qi_ranges[2][3];
}theora_quant_info;



/**The number of Huffman tables used by Theora.*/
#define OC_NHUFFMAN_TABLES (80)
/**The number of DCT token values in each table.*/
#define OC_NDCT_TOKENS     (32)

/**A Huffman code for a Theora DCT token.
 * Each set of Huffman codes in a given table must form a complete, prefix-free
 *  code.
 * There is no requirement that all the tokens in a table have a valid code,
 *  but the current encoder is not optimized to take advantage of this.
 * If there is not at least one table with a code for every token in each of
 *  the five groups of 16 tables, then the encoder may fail to encode certain
 *  frames.
 * The complete table in the first group of 16 does not have to be in the same
 *  place as the complete table in the other groups, but the complete tables in
 *  the remaining four groups must all be in the same place.*/
typedef struct{
  /**The bit pattern for the code, with the LSbit of the pattern aligned in
   *   the LSbit of the word.*/
  ogg_uint32_t pattern;
  /**The number of bits in the code.
   * This must be between 0 and 32, inclusive.*/
  int          nbits;
}theora_huff_code;



/**\defgroup basefuncs Functions Shared by Encode and Decode*/
/*@{*/
/**\name Basic shared functions*/
/*@{*/
/**Retrieves a human-readable string to identify the library vendor and
 *  version.
 * \return the version string.*/
extern const char *theora_version_string(void);
/**Retrieves the library version number.
 * This is the highest bitstream version that the encoder library will produce,
 *  or that the decoder library can decode.
 * This number is composed of a 16-bit major version, 8-bit minor version
 * and 8 bit sub-version, composed as follows:
 * \code
 * (VERSION_MAJOR<<16)+(VERSION_MINOR<<8)+(VERSION_SUBMINOR)
 * \endcode
 * \return the version number.*/
extern ogg_uint32_t theora_version_number(void);
/**Converts a granule position to an absolute frame number.
 * The granule position is interpreted in the context of a given
 *  #theora_enc_ctx or #theora_dec_ctx handle (either will suffice).
 * \param _encdec  A previously allocated #theora_enc_ctx or #theora_dec_ctx
 *                  handle.
 * \param _granpos The granule position to convert.
 * \returns The absolute frame number corresponding to \a _granpos.
 * \retval -1 The given granule position was invalid (i.e. negative).*/
extern ogg_int64_t theora_granule_frame(void *_encdec,ogg_int64_t _granpos);
/**Converts a granule position to an absolute time in seconds.
 * The granule position is interpreted in the context of a given
 *  #theora_enc_ctx or #theora_dec_ctx handle (either will suffice).
 * \param _encdec  A previously allocated #theora_enc_ctx or #theora_dec_ctx
 *                  handle.
 * \param _granpos The granule position to convert.
 * \return The absolute time in seconds corresponding to \a _granpos.
 * \retval -1 The given granule position was invalid (i.e. negative).*/
extern double theora_granule_time(void *_encdec,ogg_int64_t _granpos);
/**Determines whether a Theora packet is a header or not.
 * This function does no verification beyond checking the packet type bit, so
 *  it should not be used for bitstream identification; use
 *  theora_decode_headerin() for that.
 * \param _op An <tt>ogg_packet</tt> containing encoded Theora data.
 * \retval 1 The packet is a header packet
 * \retval 0 The packet is a video data packet.*/
extern int theora_packet_isheader(ogg_packet *_op);
/**Determines whether a theora packet is a key frame or not.
 * This function does no verification beyond checking the packet type and
 *  key frame bits, so it should not be used for bitstream identification; use
 *  theora_decode_headerin() for that.
 * \param _op An <tt>ogg_packet</tt> containing encoded Theora data.
 * \retval 1  The packet contains a key frame.
 * \retval 0  The packet contains a delta frame.
 * \retval -1 The packet is not a video data packet.*/
extern int theora_packet_iskeyframe(ogg_packet *_op);
/*@}*/


/**\name Functions for manipulating header data*/
/*@{*/
/**Initializes a theora_info structure.
 * This should be called on a freshly allocated #theora_info structure before
 *  attempting to use it.
 * \param _info The #theora_info struct to initialize.*/
extern void theora_info_init(theora_info *_info);
/**Clears a #theora_info structure.
 * This should be called on a #theora_info structure after it is no longer
 *  needed.
 * \param _info The #theora_info struct to clear.*/
extern void theora_info_clear(theora_info *_info);

/**Initialize a #theora_comment structure.
 * This should be called on a freshly allocated #theora_comment structure
 *  before attempting to use it.
 * \param _tc The #theora_comment struct to initialize.*/
extern void theora_comment_init(theora_comment *_tc);
/**Add a comment to an initialized #theora_comment structure.
 * \note Neither theora_comment_add() nor theora_comment_add_tag() support
 *  comments containing null values, although the bitstream format does
 *  support them.
 * To add such comments you will need to manipulate the #theora_comment
 *  structure directly.
 * \param _tc      The #theora_comment struct to add the comment to.
 * \param _comment Must be a null-terminated UTF-8 string containing the
 *                  comment in "TAG=the value" form.*/
extern void theora_comment_add(theora_comment *_tc, char *_comment);
/**Add a comment to an initialized #theora_comment structure.
 * \note Neither theora_comment_add() nor theora_comment_add_tag() support
 *  comments containing null values, although the bitstream format does
 *  support them.
 * To add such comments you will need to manipulate the #theora_comment
 *  structure directly.
 * \param _tc  The #theora_comment struct to add the comment to.
 * \param _tag A null-terminated string containing the tag  associated with
 *              the comment.
 * \param _val The corresponding value as a null-terminated string.*/
extern void theora_comment_add_tag(theora_comment *_tc,char *_tag,char *_val);
/**Look up a comment value by its tag.
 * \param _tc    An initialized #theora_comment structure.
 * \param _tag   The tag to look up.
 * \param _count The instance of the tag.
 *               The same tag can appear  multiple times, each with a distinct
 *                value, so an index is required to retrieve them all.
 *               The order these values appear in is significant and should be
 *                preserved.
 *               Use theora_comment_query_count() to get the legal range for
 *                the \a _count parameter.
 * \return A pointer to the queried tag's value.
 * \retval NULL If no matching tag is found.*/
extern char *theora_comment_query(theora_comment *_tc,char *_tag,int _count);
/**Look up the number of instances of a tag.
 * Call this first when querying for a specific tag and then iterate over the
 *  number of instances with separate calls to theora_comment_query() to
 *   retrieve all instances in order.
 * \param _tc    An initialized #theora_comment structure.
 * \param _tag   The tag to look up.
 * \return The number on instances of this particular tag.*/
extern int theora_comment_query_count(theora_comment *_tc,char *_tag);
/**Clears a #theora_comment structure.
 * This should be called on a #theora_comment structure after it is no longer
 *  needed.
 * It will free all memory used by the structure members.
 * \param _tc The #theora_comment struct to clear.*/
extern void theora_comment_clear(theora_comment *_tc);
/*@}*/
/*@}*/



#if defined(__cplusplus)
}
#endif

#endif
