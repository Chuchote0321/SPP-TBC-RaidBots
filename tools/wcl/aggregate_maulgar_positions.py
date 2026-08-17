#!/usr/bin/env python3
"""Aggregate High King Maulgar WCL position samples into robust role anchors.

Input is an exported CSV of already-authorized WCL positional events. This tool
never logs in to WCL and never fabricates missing coordinates.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import numpy as np
import pandas as pd

REQUIRED = {
    "report_code", "fight_id", "anchor_role", "t_ms", "x", "y", "accepted"
}

def robust_role_summary(df: pd.DataFrame) -> pd.DataFrame:
    df = df.copy()
    df = df[df["accepted"].astype(str).str.lower().isin({"1","true","yes","y"})]
    df["x"] = pd.to_numeric(df["x"], errors="coerce")
    df["y"] = pd.to_numeric(df["y"], errors="coerce")
    df = df.dropna(subset=["x","y","anchor_role"])

    rows=[]
    for role,g in df.groupby("anchor_role"):
        xmed=float(g.x.median()); ymed=float(g.y.median())
        dx=(g.x-xmed).abs(); dy=(g.y-ymed).abs()
        xmad=float(dx.median()); ymad=float(dy.median())
        # Coordinate-wise conservative MAD filter; preserve data when MAD=0.
        mask=np.ones(len(g),dtype=bool)
        if xmad>0: mask &= (dx <= 3.5*xmad).to_numpy()
        if ymad>0: mask &= (dy <= 3.5*ymad).to_numpy()
        h=g.loc[mask]
        rows.append({
            "anchor_role":role,
            "n_raw":len(g),
            "n_kept":len(h),
            "x_median":float(h.x.median()) if len(h) else xmed,
            "y_median":float(h.y.median()) if len(h) else ymed,
            "x_mad":xmad,"y_mad":ymad,
            "reports":h.report_code.nunique() if len(h) else 0,
        })
    return pd.DataFrame(rows).sort_values("anchor_role")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("csv", type=Path)
    ap.add_argument("--out", type=Path, default=Path("wcl_role_anchors.csv"))
    args=ap.parse_args()
    df=pd.read_csv(args.csv)
    missing=REQUIRED-set(df.columns)
    if missing:
        raise SystemExit(f"missing columns: {sorted(missing)}")
    if df.empty:
        raise SystemExit("input contains no positional samples; refusing to emit fake anchors")
    out=robust_role_summary(df)
    out.to_csv(args.out,index=False)
    print(out.to_string(index=False))

if __name__ == "__main__":
    main()
