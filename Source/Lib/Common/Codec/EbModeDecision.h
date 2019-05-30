/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbModeDecision_h
#define EbModeDecision_h

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbPredictionUnit.h"
#include "EbSyntaxElements.h"
#include "EbPictureBufferDesc.h"
#include "EbAdaptiveMotionVectorPrediction.h"
#include "EbPictureOperators.h"
#include "EbNeighborArrays.h"
#ifdef __cplusplus
extern "C" {
#endif
#define ENABLE_AMVP_MV_FOR_RC_PU    0
#define MAX_MB_PLANE                3
#define MAX_MPM_CANDIDATES          3
#define MERGE_PENALTY                          10

    // Create incomplete struct definition for the following function pointer typedefs
    struct ModeDecisionCandidateBuffer;
    struct ModeDecisionContext;

    /**************************************
    * Mode Decision Candidate
    **************************************/
    typedef struct ModeDecisionCandidate
    {
        // *Warning - this struct has been organized to be cache efficient when being
        //    constructured in the function GenerateAmvpMergeInterIntraMdCandidatesCU.
        //    Changing the ordering could affect performance
        union {
            struct {
                unsigned                        me_distortion : 20;
                unsigned                        distortion_ready : 1;
                unsigned                        : 2;
                unsigned                        intra_luma_mode : 8; // HEVC mode, use pred_mode for AV1
            };
            uint32_t ois_results;
        };
        union {
            struct {
                union {
                    struct {
                        int16_t              motion_vector_xl0;  //Note: Do not change the order of these fields
                        int16_t              motion_vector_yl0;
                    };
                    uint32_t mvs_l0;
                };
                union {
                    struct {
                        int16_t              motion_vector_xl1;  //Note: Do not change the order of these fields
                        int16_t              motion_vector_yl1;
                    };
                    uint32_t mvs_l1;
                };
            };
            uint64_t mvs;
        };

        uint8_t                                skip_flag;
        EbBool                                 merge_flag;
        uint16_t                               count_non_zero_coeffs;
        uint8_t                                type;
        // MD Rate Estimation Ptr
        MdRateEstimationContext             *md_rate_estimation_ptr; // 64 bits
        uint64_t                               fast_luma_rate;
        uint64_t                               fast_chroma_rate;
        uint64_t                               chroma_distortion;
        uint64_t                               chroma_distortion_inter_depth;
        uint32_t                               luma_fast_distortion;
        uint32_t                               full_distortion;

        EbPtr                                 prediction_context_ptr;
        PictureControlSet                   *picture_control_set_ptr;
        EbPredDirection                        prediction_direction[MAX_NUM_OF_PU_PER_CU]; // 2 bits // Hsan: does not seem to be used why not removed ?

        int16_t                                motion_vector_pred_x[MAX_NUM_OF_REF_PIC_LIST]; // 16 bits // Hsan: does not seem to be used why not removed ?
        int16_t                                motion_vector_pred_y[MAX_NUM_OF_REF_PIC_LIST]; // 16 bits // Hsan: does not seem to be used why not removed ?
        uint8_t                                motion_vector_pred_idx[MAX_NUM_OF_REF_PIC_LIST]; // 2 bits
        uint8_t                                block_has_coeff;             // ?? bit - determine empirically
        uint8_t                                u_has_coeff;               // ?? bit
        uint8_t                                v_has_coeff;               // ?? bit
        uint32_t                               y_has_coeff;                // Issue, should be less than 32

        PredictionMode                         pred_mode; // AV1 mode, no need to convert
        uint8_t                                drl_index;
        uint8_t                                use_intrabc;
        // Intra Mode
        int32_t                                angle_delta[PLANE_TYPES];
        EbBool                                 is_directional_mode_flag;
        EbBool                                 is_directional_chroma_mode_flag;
#if !SEARCH_UV_CLEAN_UP
        EbBool                                 use_angle_delta;
#endif
        uint32_t                               intra_chroma_mode; // AV1 mode, no need to convert

        // Index of the alpha Cb and alpha Cr combination
        int32_t                                cfl_alpha_idx;
        // Joint sign of alpha Cb and alpha Cr
        int32_t                                cfl_alpha_signs;

        // Inter Mode
        PredictionMode                         inter_mode;
        EbBool                                 is_compound;
        uint32_t                               pred_mv_weight;
        uint8_t                                ref_frame_type;
        uint8_t                                ref_mv_index;
#if MRP_MD
        int8_t                                 ref_frame_index_l0;
        int8_t                                 ref_frame_index_l1;
#endif
        EbBool                                 is_new_mv;
        EbBool                                 is_zero_mv;
#if ATB_TX_TYPE_SUPPORT_PER_TU
        TxType                                 transform_type[MAX_TXB_COUNT];
        TxType                                 transform_type_uv;
#else
        TxType                                 transform_type[PLANE_TYPES];
#endif
        MacroblockPlane                        candidate_plane[MAX_MB_PLANE];
        uint16_t                               eob[MAX_MB_PLANE][MAX_TXB_COUNT];
#if ATB_DC_CONTEXT_SUPPORT_1
        int32_t                                quantized_dc[3][MAX_TXB_COUNT];
#else
        int32_t                                quantized_dc[3];
#endif
        uint32_t                               interp_filters;
        uint8_t                                tu_width;
        uint8_t                                tu_height;
        MotionMode                            motion_mode;
        uint16_t                               num_proj_ref;
        EbBool                                 local_warp_valid;
        EbWarpedMotionParams                   wm_params;
#if ATB_SUPPORT
        uint8_t                                tx_depth;
#endif
    } ModeDecisionCandidate;


    /**************************************
    * Function Ptrs Definitions
    **************************************/
    typedef EbErrorType(*EbPredictionFunc)(
        struct ModeDecisionContext           *context_ptr,
        PictureControlSet                    *picture_control_set_ptr,
        struct ModeDecisionCandidateBuffer   *candidate_buffer_ptr,
        EbAsm                                   asm_type);
    typedef uint64_t(*EbFastCostFunc)(
        CodingUnit                           *cu_ptr,
        struct ModeDecisionCandidate         *candidateBuffer,
        uint32_t                                qp,
        uint64_t                                luma_distortion,
        uint64_t                                chroma_distortion,
        uint64_t                                lambda,
        EbBool                                  use_ssd,
        PictureControlSet                    *picture_control_set_ptr,
        CandidateMv                            *ref_mv_stack,
        const BlockGeom                        *blk_geom,
        uint32_t                                miRow,
        uint32_t                                miCol,
#if MRP_COST_EST
        uint8_t                                 md_pass,
#endif
        uint32_t                                left_neighbor_mode,
        uint32_t                                top_neighbor_mode);

    typedef EbErrorType(*EB_FULL_COST_FUNC)(
        LargestCodingUnit                    *sb_ptr,
        CodingUnit                           *cu_ptr,
        uint32_t                                cu_size,
        uint32_t                                cu_size_log2,
        struct ModeDecisionCandidateBuffer   *candidate_buffer_ptr,
        uint32_t                                qp,
        uint64_t                               *y_distortion,
        uint64_t                               *cb_distortion,
        uint64_t                               *cr_distortion,
        uint64_t                                lambda,
        uint64_t                                lambda_chroma,
        uint64_t                               *y_coeff_bits,
        uint64_t                               *cb_coeff_bits,
        uint64_t                               *cr_coeff_bits,
        uint32_t                                transform_size,
        uint32_t                                transform_chroma_size,
        PictureControlSet                    *picture_control_set_ptr);
    typedef EbErrorType(*EbAv1FullCostFunc)(
        PictureControlSet                    *picture_control_set_ptr,
        struct ModeDecisionContext           *context_ptr,
        struct ModeDecisionCandidateBuffer   *candidate_buffer_ptr,
        CodingUnit                           *cu_ptr,
        uint64_t                               *y_distortion,
        uint64_t                               *cb_distortion,
        uint64_t                               *cr_distortion,
        uint64_t                                lambda,
        uint64_t                               *y_coeff_bits,
        uint64_t                               *cb_coeff_bits,
        uint64_t                               *cr_coeff_bits,
        BlockSize                               bsize);

    typedef EbErrorType(*EB_FULL_LUMA_COST_FUNC)(
        CodingUnit                           *cu_ptr,
        uint32_t                                cu_size,
        uint32_t                                cu_size_log2,
        struct ModeDecisionCandidateBuffer   *candidate_buffer_ptr,
        uint64_t                               *y_distortion,
        uint64_t                                lambda,
        uint64_t                               *y_coeff_bits,
        uint32_t                                transform_size);
    /**************************************
    * Mode Decision Candidate Buffer
    **************************************/
    typedef struct IntraChromaCandidateBuffer 
    {
        uint32_t                              mode;
        uint64_t                              cost;
        uint64_t                              distortion;
        EbPictureBufferDesc                  *prediction_ptr;
        EbPictureBufferDesc                  *residual_ptr;
    } IntraChromaCandidateBuffer;

    /**************************************
    * Mode Decision Candidate Buffer
    **************************************/
    typedef struct ModeDecisionCandidateBuffer 
    {
        // Candidate Ptr
        ModeDecisionCandidate                *candidate_ptr;

        // Video Buffers
        EbPictureBufferDesc                  *prediction_ptr;
        EbPictureBufferDesc                  *prediction_ptr_temp;
        EbPictureBufferDesc                  *cfl_temp_prediction_ptr;
        EbPictureBufferDesc                  *residual_quant_coeff_ptr;// One buffer for residual and quantized coefficient
        EbPictureBufferDesc                  *recon_coeff_ptr;
        EbPictureBufferDesc                  *residual_ptr;

        // *Note - We should be able to combine the recon_coeff_ptr & recon_ptr pictures (they aren't needed at the same time)
        EbPictureBufferDesc                  *recon_ptr;

        // Distortion (SAD)
        uint64_t                                residual_luma_sad;
        uint64_t                                full_lambda_rate;
        uint64_t                                full_cost_luma;
                                               
        // Costs                               
        uint64_t                               *fast_cost_ptr;
        uint64_t                               *full_cost_ptr;
        uint64_t                               *full_cost_skip_ptr;
        uint64_t                               *full_cost_merge_ptr;
        //                                     
        uint64_t                                cb_coeff_bits;
        uint64_t                                cb_distortion[2];
        uint64_t                                cr_coeff_bits;
        uint64_t                                cr_distortion[2];
        uint64_t                                y_full_distortion[DIST_CALC_TOTAL];
        uint64_t                                y_coeff_bits;

    } ModeDecisionCandidateBuffer;

    /**************************************
    * Extern Function Declarations
    **************************************/
    extern EbErrorType mode_decision_candidate_buffer_ctor(
        ModeDecisionCandidateBuffer **buffer_dbl_ptr,
        uint64_t                       *fast_cost_ptr,
        uint64_t                       *full_cost_ptr,
        uint64_t                       *full_cost_skip_ptr,
        uint64_t                       *full_cost_merge_ptr
    );
    uint8_t product_full_mode_decision(
        struct ModeDecisionContext   *context_ptr,
        CodingUnit                   *cu_ptr,
        uint8_t                         bwidth,
        uint8_t                         bheight,
        ModeDecisionCandidateBuffer **buffer_ptr_array,
        uint32_t                        candidate_total_count,
        uint8_t                        *best_candidate_index_array,
        uint32_t                       *best_intra_mode);
    void sort_fast_loop_candidates(
        struct ModeDecisionContext   *context_ptr,
        uint32_t                        buffer_total_count,
        ModeDecisionCandidateBuffer **buffer_ptr_array,
        uint8_t                        *best_candidate_index_array,
        uint8_t                        *sorted_candidate_index_array,
        uint64_t                       *ref_fast_cost);
    typedef EbErrorType(*EB_INTRA_4x4_FAST_LUMA_COST_FUNC)(
        struct ModeDecisionContext           *context_ptr,
        uint32_t                                pu_index,
        ModeDecisionCandidateBuffer   *candidate_buffer_ptr,
        uint64_t                                luma_distortion,
        uint64_t                                lambda);

    typedef EbErrorType(*EB_INTRA_4x4_FULL_LUMA_COST_FUNC)(
        ModeDecisionCandidateBuffer   *candidate_buffer_ptr,
        uint64_t                               *y_distortion,
        uint64_t                                lambda,
        uint64_t                               *y_coeff_bits,
        uint32_t                                transform_size);

    typedef EbErrorType(*EB_FULL_NXN_COST_FUNC)(
        PictureControlSet                    *picture_control_set_ptr,
        ModeDecisionCandidateBuffer   *candidate_buffer_ptr,
        uint32_t                                qp,
        uint64_t                               *y_distortion,
        uint64_t                               *cb_distortion,
        uint64_t                               *cr_distortion,
        uint64_t                                lambda,
        uint64_t                                lambda_chroma,
        uint64_t                               *y_coeff_bits,
        uint64_t                               *cb_coeff_bits,
        uint64_t                               *cr_coeff_bits,
        uint32_t                                transform_size);
    struct CodingLoopContext_s;
#if MRP_LIST_REF_IDX_TYPE_LT
    /*
      |-------------------------------------------------------------|
      | ref_idx          0            1           2            3       |
      | List0            LAST        LAST2        LAST3        GOLD    |
      | List1            BWD            ALT2            ALT                |
      |-------------------------------------------------------------|
    */
#define INVALID_REF 0xF
#if  MCP_4XN_FIX 
    uint8_t get_ref_frame_idx(uint8_t ref_type);
#else
    extern uint8_t get_ref_frame_idx(uint8_t list, uint8_t ref_type);
#endif
    extern MvReferenceFrame svt_get_ref_frame_type(uint8_t list, uint8_t ref_idx);
#endif
#if INJ_MVP
    uint8_t get_list_idx(uint8_t ref_type);
#endif
#ifdef __cplusplus
}
#endif
#endif // EbModeDecision_h