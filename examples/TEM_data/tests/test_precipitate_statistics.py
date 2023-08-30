import dlite

from paths import outdir


statfile = outdir / "precipitate_statistics.csv"
PS = dlite.get_instance("http://onto-ns.com/meta/0.1/PrecipitateStatistics")
ps = PS(dimensions={"nconditions": 1})

#header = [
#    f"{p.name}({p.unit})" if p.unit else p.name
#    for p in ps.meta.properties["properties"]
#]
ps.save("csv", statfile, "mode=w")
