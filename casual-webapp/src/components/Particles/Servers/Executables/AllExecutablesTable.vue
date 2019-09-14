<template>
    <div>
        <table class="table-style">
            <thead>
            <tr>
                <th scope="col">Alias</th>
                <th scope="col">Note</th>
                <th scope="col">Path</th>
                <th scope="col">Arguments</th>
                <th scope="col">Memberships</th>
            </tr>
            </thead>
            <tbody>

            <tr v-for="executable in executables" :key="executable.alias">

                <td>
                    <router-link :to="{name: 'executable', query: {alias: executable.alias}}">
                        <a>{{executable.alias || '-'}}</a>
                    </router-link>
                </td>
                <td>{{executable.note}}</td>
                <td>{{executable.path}}</td>
                <td>{{executable.arguments.join(" ")}}</td>
                <td>{{executable.memberships.length}}</td>

            </tr>

            </tbody>
        </table>
    </div>
</template>

<script>
    export default {
        name: "AllExecutablesTable",
        props: ['executables'],
        methods: {},
    }
</script>

<style scoped>

</style>