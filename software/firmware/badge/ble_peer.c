#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gap.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_peer.h"

#include "badge.h"

ble_peer_entry ble_peer_list[BLE_PEER_LIST_SIZE];

mutex_t peer_mutex;

static THD_WORKING_AREA(waPeerThread, 128);
static THD_FUNCTION(peerThread, arg)
{
	ble_peer_entry * p;
	int i;
	(void)arg;

	chRegSetThreadName ("PeerEvent");

	while (1) {
		chThdSleepMilliseconds (1000);

		osalMutexLock (&peer_mutex);

		for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
			p = &ble_peer_list[i];

			if (p->ble_used == 0)
				continue;

			p->ble_ttl--;
			/* If this entry timed out, nuke it */
			if (p->ble_ttl == 0)
				memset (p, 0, sizeof(ble_peer_entry));
		}

		osalMutexUnlock (&peer_mutex);
	}

	/* NOTREACHED */
	return;
}

void
blePeerAdd (uint8_t * peer_addr, uint8_t * data, uint8_t len, int8_t rssi)
{
	ble_peer_entry * p;
	ble_ides_game_state_t * s;
	int firstfree = -1;
	uint8_t * d;
	uint8_t l;
	int i;

	osalMutexLock (&peer_mutex);

	/* Check for duplicates */

	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		p = &ble_peer_list[i];
		if (p->ble_used == 0 && firstfree == -1)
			firstfree = i;

		/*
		 * If we find a duplicate address, then it means we've
		 * received an advertisement or scan response for a peer
		 * That we're already familiar with. That's ok: we can
		 * just fall through to update its info and reset its TTL.
		 */

		if (memcmp (peer_addr, p->ble_peer_addr, 6) == 0) {
			firstfree = i;
			break;
		}
	}

	/* Not a duplicate, but there's no more room for new peers. :( */

	if (firstfree == -1) {
		osalMutexUnlock (&peer_mutex);
		return;
	}

	/* New entry and we have a free slot; populate it. */

	p = &ble_peer_list[firstfree];

	/* Check for name. */

	d = data;
	l = len;

	if (bleGapAdvBlockFind (&d, &l,
	    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME) == NRF_SUCCESS) {
		memset (p->ble_peer_name, 0, BLE_PEER_NAME_MAX);
		memcpy (p->ble_peer_name, d, l);
	} else {
		if (p->ble_peer_name[0] == 0x0)
			memcpy (p->ble_peer_name, "<none>", 7);
	}

	/* Now check for game state */

	d = data;
	l = len;

	if (bleGapAdvBlockFind (&d, &l,
	    BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA) == NRF_SUCCESS) {
		s = (ble_ides_game_state_t *)d;
		if (s->ble_ides_company_id == BLE_COMPANY_ID_IDES) {
			memcpy (&p->ble_game_state, s,
			    sizeof(ble_ides_game_state_t));
			p->ble_isbadge = TRUE;
		} else
			p->ble_isbadge = FALSE;
	}

#ifndef BLE_PEER_SCAN_ALL

	/*
	 * We discovered that in a very densely populated BLE
	 * environment, the peer list can fill up with so
 	 * many non-badge devices that real badges never have
	 * a chance to be seen. So for now we only allow devices
	 * we know are badges into the peer list.
	 */

	if (p->ble_isbadge == FALSE)
		goto out;
#endif

	memcpy (p->ble_peer_addr, peer_addr, 6);
	p->ble_rssi = rssi;
	p->ble_ttl = BLE_PEER_LIST_TTL;
	p->ble_used = 1;

#ifndef BLE_PEER_SCAN_ALL
out:
#endif
	osalMutexUnlock (&peer_mutex);

	return;
}

ble_peer_entry *
blePeerFind (uint8_t * peer_addr)
{
	ble_peer_entry * p;
	int i;

	osalMutexLock (&peer_mutex);
	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		p = &ble_peer_list[i];
		if (p->ble_used == 0)
			continue;
		if (memcmp (peer_addr, p->ble_peer_addr, 6) == 0)
			break;
	}
	if (i == BLE_PEER_LIST_SIZE)
		p = NULL;
	osalMutexUnlock (&peer_mutex);

	return (p);
}

void
blePeerShow (void)
{
	ble_peer_entry * p;
	ble_ides_game_state_t * t;
	int i;

	osalMutexLock (&peer_mutex);
	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		p = &ble_peer_list[i];

		if (p->ble_used == 0)
			continue;

		printf ("[%02x:%02x:%02x:%02x:%02x:%02x] ",
		    p->ble_peer_addr[5], p->ble_peer_addr[4],
		    p->ble_peer_addr[3], p->ble_peer_addr[2],
		    p->ble_peer_addr[1], p->ble_peer_addr[0]);
		printf ("[%s] ", p->ble_peer_name);
		printf ("[%d] ", p->ble_rssi);
		printf ("[%d] ", p->ble_ttl);
		if (p->ble_isbadge == TRUE) {
			t = &p->ble_game_state;
			printf ("Badge: X/Y: %d/%d SHIPTYPE: %d XP: %d"
			    " RANK: %d INCOMBAT: %d",
			    t->ble_ides_x, t->ble_ides_y, t->ble_ides_ship_type,
					t->ble_ides_xp,t->ble_ides_level, t->ble_ides_incombat);
		}
		printf ("\n");
	}

	osalMutexUnlock (&peer_mutex);

	return;
}

void
blePeerLock (void)
{
	osalMutexLock (&peer_mutex);
	return;
}

void
blePeerUnlock (void)
{
	osalMutexUnlock (&peer_mutex);
	return;
}

void
blePeerStart (void)
{
	memset (ble_peer_list, 0, sizeof(ble_peer_list));
	osalMutexObjectInit (&peer_mutex);

	chThdCreateStatic (waPeerThread, sizeof(waPeerThread),
	    NORMALPRIO, peerThread, NULL);

	return;
}
