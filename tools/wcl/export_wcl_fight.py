#!/usr/bin/env python3
"""Export one public Warcraft Logs fight with positional resources.

Credentials are read from environment variables only:
  WCL_CLIENT_ID
  WCL_CLIENT_SECRET

Do not paste the client secret into chat. This script uses the official OAuth
client-credentials flow for public report data.
"""
from __future__ import annotations
import argparse, json, os, time
from pathlib import Path
import requests

TOKEN_URL = "https://www.warcraftlogs.com/oauth/token"
API_URL = "https://www.warcraftlogs.com/api/v2/client"

QUERY = r"""
query FightAndEvents($code:String!, $fight:Int!, $start:Float) {
  reportData {
    report(code:$code) {
      fights(fightIDs:[$fight]) {
        id name startTime endTime kill difficulty friendlyPlayers friendlySpecs
        boundingBox { minX maxX minY maxY }
      }
      masterData { actors { id name type subType gameID petOwner } }
      events(fightIDs:[$fight], dataType:All, includeResources:true,
             useActorIDs:true, startTime:$start, limit:10000) {
        data
        nextPageTimestamp
      }
    }
  }
}
"""

def token() -> str:
    cid=os.environ.get("WCL_CLIENT_ID")
    secret=os.environ.get("WCL_CLIENT_SECRET")
    if not cid or not secret:
        raise SystemExit("set WCL_CLIENT_ID and WCL_CLIENT_SECRET in the environment")
    r=requests.post(TOKEN_URL, auth=(cid,secret), data={"grant_type":"client_credentials"}, timeout=30)
    r.raise_for_status()
    return r.json()["access_token"]

def gql(tok: str, variables: dict) -> dict:
    r=requests.post(API_URL, headers={"Authorization":f"Bearer {tok}"},
                    json={"query":QUERY,"variables":variables}, timeout=60)
    r.raise_for_status()
    payload=r.json()
    if payload.get("errors"):
        raise RuntimeError(json.dumps(payload["errors"],indent=2))
    return payload["data"]

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("report_code")
    ap.add_argument("fight_id",type=int)
    ap.add_argument("--out",type=Path)
    args=ap.parse_args()
    out=args.out or Path(f"wcl_{args.report_code}_fight{args.fight_id}.json")
    tok=token(); start=None; events=[]; meta=None
    while True:
        data=gql(tok,{"code":args.report_code,"fight":args.fight_id,"start":start})
        report=data["reportData"]["report"]
        if not report: raise SystemExit("report not found or not public")
        if meta is None:
            meta={"fights":report["fights"],"actors":report["masterData"]["actors"]}
        page=report["events"]; events.extend(page.get("data") or [])
        nxt=page.get("nextPageTimestamp")
        if not nxt or nxt==start: break
        start=nxt; time.sleep(0.15)
    out.write_text(json.dumps({"report_code":args.report_code,"fight_id":args.fight_id,
                               "meta":meta,"events":events},ensure_ascii=False),encoding="utf-8")
    print(f"wrote {out}: {len(events)} events")

if __name__ == "__main__": main()
