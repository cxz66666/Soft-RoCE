#ifndef RXE_TIMELY
#define RXE_TIMELY

// TODO THIS NEED MORE TEST FOR PARAMS!
#define TIMELY_MINRTT 512 
#define TIMELY_T_LOW 600 
#define TIMELY_T_HIGH 2000 
#define TIMELY_RAI   5 
#define TIMELY_A 875
#define TIMELY_B 800


static const uint64_t timely_rdtsc_mul = 2200; // 1ns = 2.2 cpu cycles

static inline int timely_check_credit(struct rxe_qp *qp, int payload) {
    return qp->timely_rate * (rdtsc() - qp->timely_timer) < timely_rdtsc_mul * payload ? 0 : 1;
}

static inline void timely_send_check(struct rxe_qp *qp, struct rxe_pkt_info *pkt, int payload) {
    // don't need timely send check!
    if (rxe_opcode[pkt->opcode].offset[RXE_TIMELY_TIMESTAMP] == 0) {
        return;
    }
    while (!timely_check_credit(qp, payload)) {}
    qp->timely_timer = rdtsc();
    *(uint64_t *)(pkt->hdr + rxe_opcode[pkt->opcode].offset[RXE_TIMELY_TIMESTAMP]) = qp->timely_timer;
}

static inline void timely_recv_pkt(struct rxe_qp *qp, struct rxe_pkt_info *pkt, struct rxe_pkt_info *ack_pkt) {
    // add timestamp for calculate
    // need to add timestamp
    uint64_t pkt_timestamp;
    if (rxe_opcode[pkt->opcode].offset[RXE_TIMELY_TIMESTAMP] == 0) {
        return;
    }
    pkt_timestamp = *(uint64_t *)(pkt->hdr + rxe_opcode[pkt->opcode].offset[RXE_TIMELY_TIMESTAMP]);
    *(uint64_t *)(ack_pkt->hdr + rxe_opcode[IB_OPCODE_RC_ACKNOWLEDGE].offset[RXE_TIMELY_TIMESTAMP]) = pkt_timestamp;
}

static inline void timely_recv_ack(struct rxe_qp *qp, struct rxe_pkt_info *pkt) {
    uint64_t prev_rtt, new_rtt, new_rtt_diff, gradient;

    // don't need timely recv ack!
    if (qp->timely_timer == 0) {
        return;
    }

    prev_rtt = TIMELY_MINRTT;
    new_rtt = rdtsc() - *(uint64_t *)(pkt->hdr + rxe_opcode[pkt->opcode].offset[RXE_TIMELY_TIMESTAMP]);
    new_rtt_diff = new_rtt - prev_rtt;
    prev_rtt = new_rtt;

    qp->timely_rtt_diff = (qp->timely_rtt_diff * (1000 - TIMELY_A) + new_rtt_diff * TIMELY_A) / 1000;
    gradient = qp->timely_rtt_diff / TIMELY_MINRTT;

    if (new_rtt < TIMELY_T_LOW) {
        qp->timely_rate += TIMELY_RAI;
    } else if (new_rtt > TIMELY_T_HIGH) {
        qp->timely_rate = qp->timely_rate * (1000 - TIMELY_B + TIMELY_B * TIMELY_T_HIGH / new_rtt) / 1000;
    } else if (gradient <= 0) {
        qp->timely_rate += 5 * TIMELY_RAI;
    } else {
        qp->timely_rate = qp->timely_rate * (1000 - TIMELY_B * gradient) / 1000;
    }
}

#endif